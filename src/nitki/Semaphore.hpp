/* The MIT License:

Copyright (c) 2008-2013 Ivan Gagis <igagis@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE. */

// Home page: http://ting.googlecode.com



/**
 * @author Ivan Gagis <igagis@gmail.com>
 * @author Jose Luis Hidalgo <joseluis.hidalgo@gmail.com> - Mac OS X initial port
 */

#pragma once

#include "../config.hpp"
#include "../debug.hpp"
#include "../types.hpp"
#include "../Exc.hpp"


#if M_OS == M_OS_WINDOWS
#	include "../windows.hpp"

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



namespace ting{
namespace mt{



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
	void Wait(){
#if M_OS == M_OS_WINDOWS
		switch(WaitForSingleObject(this->s, DWORD(INFINITE))){
			case WAIT_OBJECT_0:
//				TRACE(<< "Semaphore::Wait(): exit" << std::endl)
				break;
			case WAIT_TIMEOUT:
				ASSERT(false)
				break;
			default:
				throw ting::Exc("Semaphore::Wait(): wait failed");
				break;
		}
#elif M_OS == M_OS_SYMBIAN
		this->s.Wait();
#elif M_OS == M_OS_MACOSX
		if(pthread_mutex_lock(&this->m) != 0){
			throw ting::Exc("Semaphore::Wait(): failed to lock the mutex");
		}
		
		if(this->v == 0){
			if(pthread_cond_wait(&this->c, &this->m) != 0){
				if(pthread_mutex_unlock(&this->m) != 0){
					ASSERT(false)
				}
				throw ting::Exc("Semaphore::Wait(): pthread_cond_wait() failed");
			}
		}
		
		--this->v;
		
		if(pthread_mutex_unlock(&this->m) != 0){
			ASSERT(false)
		}
#elif M_OS == M_OS_LINUX
		int retVal;
		do{
			retVal = sem_wait(&this->s);
		}while(retVal == -1 && errno == EINTR);
		if(retVal < 0){
			throw ting::Exc("Semaphore::Wait(): wait failed");
		}
#else
#	error "unknown OS"
#endif
	}
	


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
	bool Wait(std::uint32_t timeoutMillis);



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
	inline void Signal()noexcept{
//		TRACE(<< "Semaphore::Signal(): invoked" << std::endl)
#if M_OS == M_OS_WINDOWS
		if(ReleaseSemaphore(this->s, 1, NULL) == 0){
			ASSERT(false)
		}
#elif M_OS == M_OS_SYMBIAN
		this->s.Signal();
#elif M_OS == M_OS_MACOSX
		if(pthread_mutex_lock(&this->m) != 0){
			ASSERT(false)
			return;
		}
		
		if(this->v < std::uint32_t(-1)){
			++this->v;
		}else{
			ASSERT(false)
		}
		
		if(this->v - 1 == 0){
			//someone is waiting on the semaphore
			pthread_cond_signal(&this->c);
		}
		
		if(pthread_mutex_unlock(&this->m) != 0){
			ASSERT(false)
		}
#elif M_OS == M_OS_LINUX
		if(sem_post(&this->s) < 0){
			ASSERT(false)
		}
#else
#	error "unknown OS"
#endif
	}
};//~class Semaphore



}//~namespace
}//~namespace
