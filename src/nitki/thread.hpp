#pragma once

#include <utki/config.hpp>
#include <utki/debug.hpp>

#include <thread>
#include <mutex>

namespace nitki{

/**
 * @brief a base class for threads.
 * This class should be used as a base class for thread objects, one should override the
 * thread::run() method.
 */
class thread{
	std::thread thr;

public:
	thread(const thread&) = delete;
	thread& operator=(const thread&) = delete;
	
	thread(){}
	
	virtual ~thread()noexcept;

	/**
	 * @brief This should be overridden, this is what to be run in new thread.
	 * Pure virtual method, it is called in new thread when thread runs.
	 */
	virtual void run() = 0;

	/**
	 * @brief Start thread execution.
	 * Starts execution of the thread. thread's thread::run() method will
	 * be run as separate thread of execution.
	 * @param stackSize - size of the stack in bytes which should be allocated for this thread.
	 *                    If stackSize is 0 then system default stack size is used
	 *                    (stack size depends on underlying OS).
	 */
	void start();

	/**
	 * @brief Wait for thread to finish its execution.
	 * This function waits for the thread finishes its execution,
	 * i.e. until the thread returns from its thread::run() method.
	 */
	void join()noexcept;
};

}
