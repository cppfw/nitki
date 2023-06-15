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

#include "queue.hpp"

#include <mutex>

#if CFG_OS == CFG_OS_LINUX
#	include <sys/eventfd.h>
#	include <cstring>
#elif CFG_OS == CFG_OS_MACOSX
#	include <array>
#endif

using namespace nitki;

queue::queue() :
	opros::waitable([
#if CFG_OS == CFG_OS_MACOSX
						this
#endif
]() {
#if CFG_OS == CFG_OS_WINDOWS
		auto handle = CreateEvent(
			nullptr, // security attributes
			TRUE, // manual-reset
			FALSE, // not signalled initially
			nullptr // no name
		);
		if (handle == nullptr) {
			throw std::system_error(
				int(GetLastError()),
				std::generic_category(),
				"could not create event (Win32) for implementing Waitable"
			);
		}
		return handle;
#elif CFG_OS == CFG_OS_MACOSX
		std::array<int, 2> ends;
		if (::pipe(ends.data()) < 0) {
			throw std::system_error(
				errno,
				std::generic_category(),
				"could not create pipe (*nix) for implementing Waitable"
			);
		}
		this->pipe_end = ends[1];
		return ends[0];
#elif CFG_OS == CFG_OS_LINUX
		int event_fd = eventfd(0, EFD_NONBLOCK);
		if (event_fd < 0) {
			throw std::system_error(
				errno,
				std::generic_category(),
				"could not create eventfd (linux) for implementing Waitable"
			);
		}
		return event_fd;
#else
#	error "Unsupported OS"
#endif
	}())
{}

queue::~queue() noexcept
{
#if CFG_OS == CFG_OS_WINDOWS
	CloseHandle(this->handle);
#elif CFG_OS == CFG_OS_MACOSX
	close(this->handle);
	close(this->pipe_end);
#elif CFG_OS == CFG_OS_LINUX
	close(this->handle);
#else
#	error "Unsupported OS"
#endif
}

void queue::set_ready_to_read_state() noexcept
{
	if (this->is_ready_to_read) {
		return;
	}

#if CFG_OS == CFG_OS_WINDOWS
	if (SetEvent(this->handle) == 0) {
		ASSERT(false)
	}
#elif CFG_OS == CFG_OS_MACOSX
	{
		std::array<uint8_t, 1> one_byte_buf;
		if (write(this->pipe_end, one_byte_buf.data(), 1) != 1) {
			ASSERT(false)
		}
	}
#elif CFG_OS == CFG_OS_LINUX
	if (eventfd_write(this->handle, 1) < 0) {
		ASSERT(false)
	}
#else
#	error "Unsupported OS"
#endif

	this->is_ready_to_read = true;
}

void queue::clear_ready_to_read_state() noexcept
{
#if CFG_OS == CFG_OS_WINDOWS
	if (ResetEvent(this->handle) == 0) {
		ASSERT(false)
	}
#elif CFG_OS == CFG_OS_MACOSX
	{
		std::array<uint8_t, 1> one_byte_buf;
		if (read(this->handle, one_byte_buf.data(), 1) != 1) {
			ASSERT(false)
		}
	}
#elif CFG_OS == CFG_OS_LINUX
	{
		eventfd_t value{};
		if (eventfd_read(this->handle, &value) < 0) {
			ASSERT(false)
		}
		ASSERT(value == 1)
	}
#else
#	error "Unsupported OS"
#endif

	this->is_ready_to_read = false;
}

void queue::poke() noexcept
{
	std::lock_guard<decltype(this->mut)> mutex_guard(this->mut);

	this->set_ready_to_read_state();
}

void queue::push_back(std::function<void()> proc)
{
	std::lock_guard<decltype(this->mut)> mutex_guard(this->mut);

	this->procedures.push_back(std::move(proc));

	this->set_ready_to_read_state();
}

std::function<void()> queue::pop_front()
{
	std::lock_guard<decltype(this->mut)> mutex_guard(this->mut);

	if (this->procedures.empty()) {
		return nullptr;
	}

	if (this->procedures.size() == 1) { // if we are taking away the last message from the queue
		this->clear_ready_to_read_state();
	}

	auto ret = std::move(this->procedures.front());

	this->procedures.pop_front();

	return ret;
}

size_t queue::size() const noexcept
{
	std::lock_guard<decltype(this->mut)> mutex_guard(this->mut);

	return this->procedures.size();
}

#if CFG_OS == CFG_OS_WINDOWS
void queue::set_waiting_flags(utki::flags<opros::ready> wait_for)
{
	// It is not allowed to wait on queue for write,
	// because it is always possible to push new message to queue.
	// Error condition is not possible for queue.
	// Thus, only possible flag values are READ and 0 (NOT_READY).
	// It make no sense to wait on queue for anything else than READ,
	// so we restrict setting waiting flags to READ flag only.
	if (!wait_for.get(opros::ready::read) && !wait_for.clear(opros::ready::read).is_clear()) {
		ASSERT(false, [&](auto& o) {
			o << "wait_for = " << wait_for;
		})
		throw std::invalid_argument(
			"queue::set_waiting_flags(): wait_for should have only ready::read flag set, other values are not allowed"
		);
	}
}

utki::flags<opros::ready> queue::get_readiness_flags()
{
	// if event has triggered, then there is something to read from the queue,
	// so always return ready::read
	return utki::flags<opros::ready>(false).set(opros::ready::read);
}
#endif
