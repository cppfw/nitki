#pragma once

#include <utki/config.hpp>
#include <utki/debug.hpp>
#include <utki/Exc.hpp>


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
 * (by Semaphore::Signal()) or decremented (by Semaphore::Wait()). If the value
 * is 0 then any try to decrement it will result in execution blocking of the current thread
 * until the value is incremented so the thread will be able to
 * decrement it. If there are several threads waiting for semaphore decrement and
 * some other thread increments it then only one of the hanging threads will be
 * resumed, other threads will remain waiting for next increment.
 */
class Semaphore{
	//system dependent handle
#if M_OS == M_OS_WINDOWS
	HANDLE s;
#elif M_OS == M_OS_SYMBIAN
	RSemaphore s;
#elif M_OS == M_OS_MACOSX
	//emulate semaphore using mutex and condition variable
	pthread_mutex_t m;
	pthread_cond_t c;
	unsigned v; //current semaphore value
#elif M_OS == M_OS_LINUX
	sem_t s;
#else
#	error "unknown OS"
#endif

	//forbid copying
	Semaphore(const Semaphore& );
	Semaphore& operator=(const Semaphore& );
    
public:

	/**
	 * @brief Create the semaphore with given initial value.
	 */
	Semaphore(unsigned initialValue = 0);

	~Semaphore()noexcept;

	
	/**
	 * @brief Wait on semaphore.
	 * Decrements semaphore value. If current value is 0 then this method will wait
	 * until some other thread signals the semaphore (i.e. increments the value)
	 * by calling Semaphore::Signal() on that semaphore.
	 */
	void wait();
	


	/**
	 * @brief Wait on semaphore with timeout.
	 * Decrements semaphore value. If current value is 0 then this method will wait
	 * until some other thread signals the semaphore (i.e. increments the value)
	 * by calling Semaphore::Signal() on that semaphore.
	 * @param timeoutMillis - waiting timeout.
	 *                        If timeoutMillis is 0 (the default value) then this
	 *                        method will try to decrement the semaphore value and exit immediately.
	 * @return returns true if the semaphore value was decremented.
	 * @return returns false if the timeout was hit.
	 */
	bool wait(std::uint32_t timeoutMillis);



	/**
	 * @brief Signal the semaphore.
	 * Increments the semaphore value.
	 * The semaphore value is a 32bit unsigned integer, so it can be a pretty big values.
	 * But, if the maximum value is reached then subsequent calls to this method
	 * will not do any incrementing (because the maximum value is reached), i.e. there will
	 * be no semaphore value warp around to 0 again. Reaching such condition is
	 * considered as an error condition which, in theory, should never occur in the program.
	 * Because of that, in the debug mode (DEBUG macro defined) there are assertions to
	 * detect such a condition.
	 */
	void signal()noexcept;
};



}//~namespace
