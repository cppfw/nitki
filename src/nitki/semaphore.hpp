#pragma once

#include <utki/config.hpp>
#include <utki/debug.hpp>

#if M_OS == M_OS_WINDOWS
#	include <utki/windows.hpp>

#elif M_OS == M_OS_SYMBIAN
#	include <string.h>
#	include <e32std.h>
#	include <hal.h>

#elif M_OS == M_OS_LINUX || M_OS == M_OS_UNIX
#	include <semaphore.h>
#	include <errno.h>

#elif M_OS == M_OS_MACOSX
#	include <pthread.h>

#else
#	error "Unsupported OS"
#endif

namespace nitki{

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
class semaphore{
#if M_OS == M_OS_WINDOWS
	HANDLE s;
#elif M_OS == M_OS_SYMBIAN
	RSemaphore s;
#elif M_OS == M_OS_MACOSX
	// emulate semaphore using mutex and condition variable
	pthread_mutex_t m;
	pthread_cond_t c;
	unsigned v; // current semaphore value
#elif M_OS == M_OS_LINUX
	sem_t s;
#else
#	error "unknown OS"
#endif

	semaphore(const semaphore&) = delete;
	semaphore& operator=(const semaphore&) = delete;
    
public:

	/**
	 * @brief Create the semaphore with given initial value.
	 * @param initial_value - initial value of the semaphore.
	 */
	semaphore(unsigned initial_value = 0);

	~semaphore()noexcept;

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

}
