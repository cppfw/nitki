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

#pragma once

#include <utki/config.hpp>
#include <utki/debug.hpp>

#if CFG_OS == CFG_OS_WINDOWS
#	include <utki/windows.hpp>

#elif CFG_OS == CFG_OS_LINUX || CFG_OS == CFG_OS_UNIX
#	include <semaphore.h>
#	include <cerrno>

#elif CFG_OS == CFG_OS_MACOSX
#	include <pthread.h>

#else
#	error "Unsupported OS"
#endif

namespace nitki {

/**
 * @brief Semaphore class.
 * The semaphore is actually an unsigned integer value which can be incremented
 * (by semaphore::signal()) or decremented (by semaphore::wait()). If the value
 * is 0 then any try to decrement it will result in execution blocking of the current thread
 * until the value is incremented so the thread will be able to
 * decrement it. If there are several threads waiting for semaphore decrement and
 * some other thread increments it then only one of the hanging threads will be
 * resumed, other threads will remain waiting for next increment.
 */
class semaphore
{
#if CFG_OS == CFG_OS_WINDOWS
	HANDLE s;
#elif CFG_OS == CFG_OS_MACOSX
	// emulate semaphore using mutex and condition variable
	pthread_mutex_t m;
	pthread_cond_t c;
	unsigned v; // current semaphore value
#elif CFG_OS == CFG_OS_LINUX
	sem_t s;
#else
#	error "unknown OS"
#endif

public:
	semaphore(const semaphore&) = delete;
	semaphore& operator=(const semaphore&) = delete;

	/**
	 * @brief Create the semaphore with given initial value.
	 * @param initial_value - initial value of the semaphore.
	 */
	semaphore(unsigned initial_value = 0);

	~semaphore() noexcept;

	/**
	 * @brief Wait on semaphore.
	 * Decrements semaphore value. If current value is 0 then this method will wait
	 * until some other thread signals the semaphore (i.e. increments the value)
	 * by calling semaphore::signal() on that semaphore.
	 */
	void wait();

	/**
	 * @brief Wait on semaphore with timeout.
	 * Decrements semaphore value. If current value is 0 then this method will wait
	 * until some other thread signals the semaphore (i.e. increments the value)
	 * by calling semaphore::signal() on that semaphore.
	 * @param timeout_ms - waiting timeout.
	 *                     If timeout is 0 (the default value) then this
	 *                     method will try to decrement the semaphore value and exit immediately.
	 * @return returns true if the semaphore value was decremented.
	 * @return returns false if the timeout was hit.
	 */
	bool wait(uint32_t timeout_ms);

	/**
	 * @brief Signal the semaphore.
	 * Increments the semaphore value.
	 */
	void signal();
};

} // namespace nitki
