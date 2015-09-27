#include "Semaphore.hpp"

#if M_OS == M_OS_MACOSX
#	include <cerrno>
#	include <sys/time.h>
#endif


using namespace ting::mt;



Semaphore::Semaphore(unsigned initialValue){
#if M_OS == M_OS_WINDOWS
	if( (this->s = CreateSemaphore(NULL, initialValue, 0xffffff, NULL)) == NULL)
#elif M_OS == M_OS_SYMBIAN
	if(this->s.CreateLocal(initialValue) != KErrNone)
#elif M_OS == M_OS_MACOSX
	if(pthread_mutex_init(&this->m, 0) == 0){
		if(pthread_cond_init(&this->c, 0) == 0){
			this->v = initialValue;
			return;
		}
		pthread_mutex_destroy(&this->m);
	}
#elif M_OS == M_OS_LINUX
	if(sem_init(&this->s, 0, initialValue) < 0)
#else
#	error "unknown OS"
#endif
	{
		TRACE(<< "Semaphore::Semaphore(): failed" << std::endl)
		throw ting::Exc("Semaphore::Semaphore(): creating semaphore failed");
	}
}



Semaphore::~Semaphore()noexcept{
#if M_OS == M_OS_WINDOWS
	CloseHandle(this->s);
#elif M_OS == M_OS_SYMBIAN
	this->s.Close();
#elif M_OS == M_OS_MACOSX
	pthread_cond_destroy(&this->c);
	pthread_mutex_destroy(&this->m);
#elif M_OS == M_OS_LINUX
	sem_destroy(&this->s);
#else
#	error "unknown OS"
#endif
}



bool Semaphore::Wait(std::uint32_t timeoutMillis){
#if M_OS == M_OS_WINDOWS
	static_assert(INFINITE == 0xffffffff, "error");
	switch(WaitForSingleObject(this->s, DWORD(timeoutMillis == INFINITE ? INFINITE - 1 : timeoutMillis))){
		case WAIT_OBJECT_0:
			return true;
		case WAIT_TIMEOUT:
			return false;
		default:
			throw ting::Exc("Semaphore::Wait(u32): wait failed");
			break;
	}
#elif M_OS == M_OS_MACOSX
	struct timeval tv;
	
	gettimeofday(&tv, NULL);
	
	struct timespec ts;
	
	ts.tv_sec = tv.tv_sec;
	ts.tv_nsec = tv.tv_usec * 1000;

	ts.tv_sec += timeoutMillis / 1000;
	ts.tv_nsec += (timeoutMillis % 1000) * 1000 * 1000;
	ts.tv_sec += ts.tv_nsec / (1000 * 1000 * 1000);
	ts.tv_nsec = ts.tv_nsec % (1000 * 1000 * 1000);
	
	if(pthread_mutex_lock(&this->m) != 0){
		throw ting::Exc("Semaphore::Wait(): failed to lock the mutex");
	}

	if(this->v == 0){
		if(int err = pthread_cond_timedwait(&this->c, &this->m, &ts)){
			if(pthread_mutex_unlock(&this->m) != 0){
				ASSERT(false)
			}
			if(err == ETIMEDOUT){
				return false;
			}else{
				TRACE(<< "Semaphore::Wait(): pthread_cond_wait() failed, error code = " << err << std::endl)
				throw ting::Exc("Semaphore::Wait(): pthread_cond_wait() failed");
			}
		}
	}

	--this->v;

	if(pthread_mutex_unlock(&this->m) != 0){
		ASSERT(false)
	}
#elif M_OS == M_OS_LINUX
	//if timeoutMillis is 0 then use sem_trywait() to avoid unnecessary time calculation for sem_timedwait()
	if(timeoutMillis == 0){
		if(sem_trywait(&this->s) == -1){
			if(errno == EAGAIN){
				return false;
			}else{
				throw ting::Exc("Semaphore::Wait(u32): error: sem_trywait() failed");
			}
		}
	}else{
		timespec ts;

		if(clock_gettime(CLOCK_REALTIME, &ts) == -1){
			throw ting::Exc("Semaphore::Wait(): clock_gettime() returned error");
		}

		ts.tv_sec += timeoutMillis / 1000;
		ts.tv_nsec += (timeoutMillis % 1000) * 1000 * 1000;
		ts.tv_sec += ts.tv_nsec / (1000 * 1000 * 1000);
		ts.tv_nsec = ts.tv_nsec % (1000 * 1000 * 1000);

		if(sem_timedwait(&this->s, &ts) == -1){
			if(errno == ETIMEDOUT){
				return false;
			}else{
				throw ting::Exc("Semaphore::Wait(u32): error: sem_timedwait() failed");
			}
		}
	}
#else
#	error "unknown OS"
#endif
	return true;
}
