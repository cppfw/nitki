/*
The MIT License (MIT)

Copyright (c) 2020 Ivan Gagis <igagis@gmail.com>

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

#if M_OS == M_OS_LINUX
#	include <sys/eventfd.h>
#	include <cstring>
#endif

using namespace nitki;

queue::queue(){
	// it is always possible to post a message to the queue, so set ready to write flag
	this->readiness_flags.set(opros::ready::write);

#if M_OS == M_OS_WINDOWS
	this->event_handle = CreateEvent(
			NULL, // security attributes
			TRUE, // manual-reset
			FALSE, // not signalled initially
			NULL // no name
		);
	if(this->event_handle == NULL){
		throw std::system_error(GetLastError(), std::generic_category(), "could not create event (Win32) for implementing waitable");
	}
#elif M_OS == M_OS_MACOSX
	if(::pipe(&this->pipe_ends[0]) < 0){
		throw std::system_error(errno, std::generic_category(), "could not create pipe (*nix) for implementing waitable");
	}
#elif M_OS == M_OS_LINUX
	this->event_fd = eventfd(0, EFD_NONBLOCK);
	if(this->event_fd < 0){
		throw std::system_error(errno, std::generic_category(), "could not create eventfd (linux) for implementing waitable");
	}
#else
#	error "Unsupported OS"
#endif
}

queue::~queue()noexcept{
#if M_OS == M_OS_WINDOWS
	CloseHandle(this->event_handle);
#elif M_OS == M_OS_MACOSX
	close(this->pipe_ends[0]);
	close(this->pipe_ends[1]);
#elif M_OS == M_OS_LINUX
	close(this->event_fd);
#else
#	error "Unsupported OS"
#endif
}

void queue::push_back(std::function<void()>&& proc){
	std::lock_guard<decltype(this->mut)> mutex_guard(this->mut);

	this->procedures.push_back(std::move(proc));
	
	if(this->procedures.size() == 1){ // if it is a first message
		// Set read flag.
		// NOTE: in linux implementation with epoll(), the read
		// flag will also be set in wait_set::wait() method.
		// NOTE: set read flag before event notification/pipe write, because
		// if do it after then some other thread which was waiting on the wait_set
		// may check the read flag while it was not set yet.
		ASSERT(!this->flags().get(opros::ready::read))
		this->readiness_flags.set(opros::ready::read);

#if M_OS == M_OS_WINDOWS
		if(SetEvent(this->event_handle) == 0){
			ASSERT(false)
		}
#elif M_OS == M_OS_MACOSX
		{
			std::uint8_t oneByteBuf[1];
			if(write(this->pipe_ends[1], oneByteBuf, 1) != 1){
				ASSERT(false)
			}
		}
#elif M_OS == M_OS_LINUX
		if(eventfd_write(this->event_fd, 1) < 0){
			ASSERT(false)
		}
#else
#	error "Unsupported OS"
#endif
	}

	ASSERT(this->flags().get(opros::ready::read))
}

std::function<void()> queue::pop_front(){
	std::lock_guard<decltype(this->mut)> mutex_guard(this->mut);

	if(!this->procedures.empty()){
		ASSERT(this->flags().get(opros::ready::read))

		if(this->procedures.size() == 1){ // if we are taking away the last message from the queue
#if M_OS == M_OS_WINDOWS
			if(ResetEvent(this->event_handle) == 0){
				ASSERT(false)
				throw std::system_error(GetLastError(), std::generic_category(), "queue::wait(): ResetEvent() failed");
			}
#elif M_OS == M_OS_MACOSX
			{
				std::uint8_t oneByteBuf[1];
				if(read(this->pipe_ends[0], oneByteBuf, 1) != 1){
					throw std::system_error(errno, std::generic_category(), "queue::wait(): read() failed");
				}
			}
#elif M_OS == M_OS_LINUX
			{
				eventfd_t value;
				if(eventfd_read(this->event_fd, &value) < 0){
					throw std::system_error(errno, std::generic_category(), "queue::wait(): eventfd_read() failed");
				}
				ASSERT(value == 1)
			}
#else
#	error "Unsupported OS"
#endif
			this->readiness_flags.clear(opros::ready::read);
		}else{
			ASSERT(this->flags().get(opros::ready::read))
		}
		
		auto ret = std::move(this->procedures.front());
		
		this->procedures.pop_front();
		
		return ret;
	}
	return nullptr;
}

size_t queue::size()const noexcept{
	std::lock_guard<decltype(this->mut)> mutex_guard(this->mut);
	return this->procedures.size();
}

#if M_OS == M_OS_WINDOWS
HANDLE queue::get_handle(){
	return this->event_handle;
}

void queue::set_waiting_flags(utki::flags<opros::ready> wait_for){
	// It is not allowed to wait on queue for write,
	// because it is always possible to push new message to queue.
	// Error condition is not possible for queue.
	// Thus, only possible flag values are READ and 0 (NOT_READY)
	if(wait_for.get(opros::ready::write) || wait_for.get(opros::ready::error)){
		ASSERT_INFO(false, "wait_for = " << wait_for)
		throw std::invalid_argument("queue::set_waiting_flags(): wait_for can only have read flag set or no flags set");
	}

	this->waiting_flags = wait_for;
}

bool queue::check_signaled(){
	// error condition is not possible for queue
	ASSERT(!this->flags().get(opros::ready::error))

	return !(this->readiness_flags & this->waiting_flags).is_clear();
}

#elif M_OS == M_OS_MACOSX
int queue::get_handle(){
	return this->pipe_ends[0]; // return read end of pipe
}

#elif M_OS == M_OS_LINUX
int queue::get_handle(){
	return this->event_fd;
}

#else
#	error "Unsupported OS"
#endif
