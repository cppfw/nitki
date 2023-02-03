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

#include <mutex>
#include <thread>

#include <utki/config.hpp>
#include <utki/debug.hpp>

namespace nitki {

/**
 * @brief a base class for threads.
 * This class should be used as a base class for thread objects, one should override the
 * thread::run() method.
 */
class thread
{
	std::thread thr;

public:
	thread(const thread&) = delete;
	thread& operator=(const thread&) = delete;

	thread() = default;

	// NOLINTNEXTLINE(modernize-use-equals-default, "destructor is not trivial in debug build configuration")
	virtual ~thread() noexcept
	{
		ASSERT(!this->thr.joinable(), [](auto& o) {
			o << "~thread() destructor is called while the thread was not joined before. "
			  << "Make sure the thread is joined by calling thread::join() "
			  << "before destroying the thread object.";
		})

		// NOTE: it is incorrect to put this->join() to this destructor, because
		//       thread shall already be stopped at the moment when this destructor
		//       is called. If it is not, then the thread will be still running
		//       when part of the thread object is already destroyed, since thread object is
		//       usually a derived object from thread class and the destructor of this derived
		//       object will be called before ~thread() destructor.
	}

	/**
	 * @brief This should be overridden, this is what to be run in new thread.
	 * Pure virtual method, it is called in new thread when thread runs.
	 */
	virtual void run() = 0;

	/**
	 * @brief Start thread execution.
	 * Starts execution of the thread. thread's thread::run() method will
	 * be run as separate thread of execution.
	 */
	void start();

	/**
	 * @brief Wait for thread to finish its execution.
	 * This function waits for the thread finishes its execution,
	 * i.e. until the thread returns from its thread::run() method.
	 */
	void join() noexcept;
};

} // namespace nitki
