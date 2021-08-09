/*
The MIT License (MIT)

Copyright (c) 2015-2021 Ivan Gagis <igagis@gmail.com>

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
#include <utki/spin_lock.hpp>

#include <opros/wait_set.hpp>

#include <deque>
#include <functional>

namespace nitki{

/**
 * @brief Procedure queue.
 * Procedure queue is used for communication of separate threads by
 * means of sending procedures to each other. Thus, when one thread sends a procedure to another one,
 * it asks that another thread to execute some code portion, i.e. procedure.
 * NOTE: queue implements waitable interface which means that it can be used in conjunction
 * with opros::wait_set. But, note, that the implementation of the waitable is that it
 * shall only be used to wait for read. If you are trying to wait for write the behavior will be
 * undefined.
 */
class queue : public opros::waitable{
	mutable utki::spin_lock mut;

	std::deque<std::function<void()>> procedures;
	
#if M_OS == M_OS_WINDOWS
	HANDLE event_handle; // use Event to implement waitable on Windows
#elif M_OS == M_OS_MACOSX
	int pipe_ends[2]; // use pipe to implement waitable in *nix systems
#elif M_OS == M_OS_LINUX
	int event_fd; // use eventfd()
#else
#	error "Unsupported OS"
#endif

	queue(const queue&) = delete;
	queue& operator=(const queue&) = delete;

public:
	/**
	 * @brief Constructor, creates empty message queue.
	 */
	queue();

	/**
	 * @brief Destructor.
	 * When called, it also destroys all procedures on the queue.
	 */
	~queue()noexcept;

	/**
	 * @brief Pushes a new procedure to the end of the queue.
	 * @param proc - the procedure to push into the queue.
	 */
	void push_back(std::function<void()>&& proc);

	/**
	 * @brief Get procedure from queue, does not block if no procedures queued.
	 * This method gets a procedure from the front of the queue. If there are no procedures on the queue
	 * it will return nullptr.
	 * @return procedure.
	 * @return nullptr if there are no procedures in the queue.
	 */
	std::function<void()> pop_front();

	/**
	 * @brief Get number of procedures in the queue.
	 * This function involves mutex acquisition.
	 * @return number of procedures in the queue.
	 */
	size_t size()const noexcept;

#if M_OS == M_OS_WINDOWS
protected:
	HANDLE get_handle()override;

	utki::flags<opros::ready> waiting_flags;

	void set_waiting_flags(utki::flags<opros::ready> wait_for)override;

	bool check_signaled()override;

#elif M_OS == M_OS_LINUX
public:
	int get_handle()override;

#elif M_OS == M_OS_MACOSX
public:
	int get_handle()override;

#else
#	error "Unsupported OS"
#endif
};

}
