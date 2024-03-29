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

#include <atomic>
#include <optional>

#include <opros/wait_set.hpp>

#include "queue.hpp"
#include "thread.hpp"

namespace nitki {

class loop_thread : public nitki::thread
{
	nitki::queue queue;

	std::atomic_bool quit_flag = false;

public:
	/**
	 * @brief wait_set of the thread.
	 * This is the wait set of waitable objects for whose readiness the
	 * thread waits on each iteration of the main loop.
	 */
	opros::wait_set wait_set;

	/**
	 * @brief Construct a new loop thread object.
	 * The actual wait_set capacity will be more than requested by 1.
	 * This is because internal loop_thread::queue is added to the wait_set as well.
	 * The internal loop_thread::queue is added with nullptr user_data, so it will be easy
	 * to identify the queue from the list returned by wait_set::get_triggered().
	 *
	 * @param wait_set_capacity - requested capacity of the thread's wait_set.
	 */
	loop_thread(unsigned wait_set_capacity);

	~loop_thread() override;

	loop_thread(const loop_thread&) = delete;
	loop_thread& operator=(const loop_thread&) = delete;

	loop_thread(loop_thread&&) = delete;
	loop_thread& operator=(loop_thread&&) = delete;

	/**
	 * @brief Main loop of the thread.
	 * The run() method is overridden to implement main loop with procedure queue
	 * and quit flag.
	 */
	void run() final;

	/**
	 * @brief Loop iteration procedure.
	 * This function is called every main loop iteration, right before waiting on the
	 * wait_set and running thread's queue procedures.
	 * @return desired triggering objects waiting timeout in milliseconds for next
	 * iteration.
	 * @return empty std::optional for infinite waiting for triggering objects.
	 */
	virtual std::optional<uint32_t> on_loop() = 0;

	/**
	 * @brief Thread exit procedure.
	 * This function is called from within the thread after it has exited the main loop,
	 * right before exiting the thread's run() function.
	 */
	virtual void on_quit() {}

	/**
	 * @brief Request this thread to quit.
	 */
	void quit() noexcept;

	/**
	 * @brief Pushes a new procedure to the end of the thread's queue.
	 * @param proc - the procedure to push into the queue.
	 */
	void push_back(std::function<void()> proc)
	{
		this->queue.push_back(std::move(proc));
	}

	/**
	 * @brief Trigger the queue ready to read.
	 * This method triggers the thread's queue to be ready to read
	 * without actually posting a procedure into the queue.
	 */
	void poke() noexcept
	{
		this->queue.poke();
	}
};

} // namespace nitki
