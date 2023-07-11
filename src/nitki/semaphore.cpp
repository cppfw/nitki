/*
The MIT License (MIT)

Copyright (c) 2015-2023 Ivan Gagis <igagis@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

/* ================ LICENSE END ================ */

#include "semaphore.hpp"

#include <ratio>

#if CFG_OS == CFG_OS_MACOSX
#	include <cerrno>
#	include <sys/time.h>
#elif CFG_OS == CFG_OS_WINDOWS
#	include <limits>
#	include <sstream>
#endif

#include <utki/util.hpp>

using namespace nitki;

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
semaphore::semaphore(unsigned initial_value)
{
#if CFG_OS == CFG_OS_WINDOWS
	using namespace std::string_literals;
	auto max_val = std::numeric_limits<LONG>::max();
	if (initial_value >= unsigned(max_val)) {
		std::stringstream ss;
		ss << "semaphore::semaphore(): initial_value cannot be >= " << unsigned(max_val);
		throw std::invalid_argument(ss.str());
	}
	this->s = CreateSemaphore(nullptr, LONG(initial_value), max_val, nullptr);
	if (!this->s)
#elif CFG_OS == CFG_OS_MACOSX
	if (pthread_mutex_init(&this->m, nullptr) == 0) {
		if (pthread_cond_init(&this->c, nullptr) == 0) {
			this->v = initial_value;
			return;
		}
		pthread_mutex_destroy(&this->m);
	}
#elif CFG_OS == CFG_OS_LINUX
	if (sem_init(&this->s, 0, initial_value) < 0)
#else
#	error "unknown OS"
#endif
	{
		LOG([&](auto& o) {
			o << "semaphore::semaphore(): failed" << std::endl;
		})
		throw std::system_error(errno, std::generic_category(), "semaphore::semaphore(): sem_init() failed");
	}
}

semaphore::~semaphore() noexcept
{
#if CFG_OS == CFG_OS_WINDOWS
	CloseHandle(this->s);
#elif CFG_OS == CFG_OS_MACOSX
	pthread_cond_destroy(&this->c);
	pthread_mutex_destroy(&this->m);
#elif CFG_OS == CFG_OS_LINUX
	sem_destroy(&this->s);
#else
#	error "unknown OS"
#endif
}

void semaphore::wait()
{
#if CFG_OS == CFG_OS_WINDOWS
	switch (WaitForSingleObject(this->s, DWORD(INFINITE))) {
		case WAIT_TIMEOUT: // NOLINT(bugprone-branch-clone)
			[[fallthrough]];
		case WAIT_ABANDONED:
			ASSERT(false)
			[[fallthrough]];
		case WAIT_OBJECT_0:
			break;
		case WAIT_FAILED:
			throw std::system_error(
				int(GetLastError()),
				std::generic_category(),
				"semaphore::wait(): WaitForSingleObject() failed"
			);
	}
#elif CFG_OS == CFG_OS_MACOSX
	if (int error = pthread_mutex_lock(&this->m)) {
		throw std::system_error(error, std::generic_category(), "semaphore::wait(): pthread_mutex_lock() failed");
	}

	if (this->v == 0) {
		if (int error = pthread_cond_wait(&this->c, &this->m)) {
			if (pthread_mutex_unlock(&this->m) != 0) {
				ASSERT(false)
			}
			throw std::system_error(error, std::generic_category(), "semaphore::wait(): pthread_cond_wait() failed");
		}
	}

	--this->v;

	if (pthread_mutex_unlock(&this->m) != 0) {
		ASSERT(false)
	}
#elif CFG_OS == CFG_OS_LINUX
	int res = 0;
	for (;;) {
		res = sem_wait(&this->s);
		if ((res == -1 && errno == EINTR)) {
			continue;
		}
		break;
	}
	if (res < 0) {
		throw std::system_error(errno, std::generic_category(), "semaphore::wait(): sem_wait() failed");
	}
#else
#	error "unknown OS"
#endif
}

bool semaphore::wait(uint32_t timeout_ms)
{
#if CFG_OS == CFG_OS_WINDOWS
	static_assert(INFINITE == std::numeric_limits<DWORD>::max(), "error");
	switch (WaitForSingleObject(this->s, DWORD(timeout_ms == INFINITE ? INFINITE - 1 : timeout_ms))) {
		case WAIT_OBJECT_0:
			return true;
		case WAIT_TIMEOUT:
			return false;
		default:
			throw std::system_error(int(GetLastError()), std::generic_category(), "semaphore::wait(): wait failed");
	}
#elif CFG_OS == CFG_OS_MACOSX
	struct timeval tv {};

	gettimeofday(&tv, nullptr);

	struct timespec ts {};

	ts.tv_sec = tv.tv_sec;
	ts.tv_nsec = static_cast<long>(tv.tv_usec) * std::milli::den;

	ts.tv_sec += timeout_ms / std::milli::den;
	ts.tv_nsec += static_cast<long>(timeout_ms % std::milli::den) * std::nano::den;
	ts.tv_sec += ts.tv_nsec / static_cast<long>(std::milli::den * std::nano::den);
	ts.tv_nsec = ts.tv_nsec % static_cast<long>(std::milli::den * std::nano::den);

	if (int error = pthread_mutex_lock(&this->m)) {
		throw std::system_error(error, std::generic_category(), "semaphore::wait(): failed to lock the mutex");
	}

	if (this->v == 0) {
		if (int error = pthread_cond_timedwait(&this->c, &this->m, &ts)) {
			if (pthread_mutex_unlock(&this->m) != 0) {
				ASSERT(false)
			}
			if (error == ETIMEDOUT) {
				return false;
			} else {
				LOG([&](auto& o) {
					o << "semaphore::wait(): pthread_cond_wait() failed, error code = " << err << std::endl;
				})
				throw std::system_error(
					error,
					std::generic_category(),
					"semaphore::wait(): pthread_cond_wait() failed"
				);
			}
		}
	}

	--this->v;

	if (pthread_mutex_unlock(&this->m) != 0) {
		ASSERT(false)
	}
#elif CFG_OS == CFG_OS_LINUX
	// if timeout is 0 then use sem_trywait() to avoid unnecessary time calculation for sem_timedwait()
	if (timeout_ms == 0) {
		if (sem_trywait(&this->s) == -1) {
			if (errno == EAGAIN) {
				return false;
			} else {
				throw std::system_error(errno, std::generic_category(), "semaphore::wait(): sem_trywait() failed");
			}
		}
	} else {
		timespec ts{};

		if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
			throw std::system_error(
				errno,
				std::generic_category(),
				"semaphore::wait(): clock_gettime() returned error"
			);
		}

		ts.tv_sec += timeout_ms / std::milli::den;
		ts.tv_nsec += long(timeout_ms % std::milli::den) * std::nano::den;
		ts.tv_sec += ts.tv_nsec / (long(std::milli::den) * std::nano::den);
		ts.tv_nsec = ts.tv_nsec % (long(std::milli::den) * std::nano::den);

		if (sem_timedwait(&this->s, &ts) == -1) {
			if (errno == ETIMEDOUT) {
				return false;
			} else {
				throw std::system_error(errno, std::generic_category(), "semaphore::wait(): sem_timedwait() failed");
			}
		}
	}
#else
#	error "unknown OS"
#endif
	return true;
}

void semaphore::signal()
{
	//		TRACE(<< "semaphore::signal(): invoked" << std::endl)
#if CFG_OS == CFG_OS_WINDOWS
	if (ReleaseSemaphore(this->s, 1, nullptr) == 0) {
		throw std::system_error(int(GetLastError()), std::generic_category(), "ReleaseSemaphore() failed");
	}
#elif CFG_OS == CFG_OS_MACOSX
	if (int error = pthread_mutex_lock(&this->m)) {
		throw std::system_error(error, std::generic_category(), "pthread_mutex_lock() failed");
	}

	if (this->v < std::uint32_t(-1)) {
		++this->v;
	} else {
		throw std::logic_error("semaphore::signal(): semaphore value is already at maximum");
	}

	if (this->v - 1 == 0) {
		// someone is waiting on the semaphore
		pthread_cond_signal(&this->c);
	}

	if (int error = pthread_mutex_unlock(&this->m)) {
		throw std::system_error(error, std::generic_category(), "pthread_mutex_unlock() failed");
	}
#elif CFG_OS == CFG_OS_LINUX
	if (sem_post(&this->s) < 0) {
		throw std::system_error(errno, std::generic_category(), "sem_post() failed");
	}
#else
#	error "unknown OS"
#endif
}
