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
	
	virtual ~thread()noexcept{
		ASSERT(
				!this->thr.joinable(),
				[](auto&o){
					o <<"~thread() destructor is called while the thread was not joined before. "
							<< "Make sure the thread is joined by calling thread::join() "
							<< "before destroying the thread object.";
				}
		)

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
	void join()noexcept;
};

}
