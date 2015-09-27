/**
 * @file Thread.hpp
 * @author Ivan Gagis <igagis@gmail.com>
 * @author Jose Luis Hidalgo <joseluis.hidalgo@gmail.com> - Mac OS X port
 * @brief Multithreading library.
 */

#pragma once

#include <utki/config.hpp>
#include <utki/debug.hpp>
#include <utki/Exc.hpp>

#include <mutex>



#if M_OS == M_OS_WINDOWS
#	include <utki/windows.hpp>

#elif M_OS == M_OS_SYMBIAN
#	include <string.h>
#	include <e32std.h>
#	include <hal.h>

#elif M_OS == M_OS_LINUX || M_OS == M_OS_MACOSX || M_OS == M_OS_UNIX
#	include <pthread.h>
#	include <unistd.h>

#	if M_OS_NAME == M_OS_NAME_SOLARIS
#		include <sched.h> // for sched_yield();
#	endif

#else
#	error "Unsupported OS"
#endif



namespace nitki{



/**
 * @brief a base class for threads.
 * This class should be used as a base class for thread objects, one should override the
 * Thread::Run() method.
 */
class Thread{

//Tread Run function
#if M_OS == M_OS_WINDOWS
	static unsigned int __stdcall runThread(void *data);
#elif M_OS == M_OS_SYMBIAN
	static TInt runThread(TAny *data);
#elif M_OS == M_OS_LINUX || M_OS == M_OS_MACOSX
	static void* runThread(void *data);
#else
#	error "Unsupported OS"
#endif


	std::mutex mutex1;


	enum class E_State{
		NEW,
		RUNNING,
		STOPPED,
		JOINED
	};
	
	volatile E_State state = E_State::NEW;

	//system dependent handle
#if M_OS == M_OS_WINDOWS
	HANDLE th;
#elif M_OS == M_OS_SYMBIAN
	RThread th;
#elif M_OS == M_OS_LINUX || M_OS == M_OS_MACOSX
	pthread_t th;
#else
#	error "Unsupported OS"
#endif

	//forbid copying
	Thread(const Thread& );
	Thread& operator=(const Thread& );

public:
	
	/**
	 * @brief Basic exception type thrown by Thread class.
	 * @param msg - human friendly exception description.
	 */
	class Exc : public utki::Exc{
	public:
		Exc(const std::string& msg) :
				utki::Exc(msg)
		{}
	};
	
	class ThreadHasAlreadyBeenStartedExc : public Exc{
	public:
		ThreadHasAlreadyBeenStartedExc() :
				Exc("The thread has already been started.")
		{}
	};
	
	Thread();
	
	
	virtual ~Thread()noexcept;



	/**
	 * @brief This should be overridden, this is what to be run in new thread.
	 * Pure virtual method, it is called in new thread when thread runs.
	 */
	virtual void run() = 0;



	/**
	 * @brief Start thread execution.
	 * Starts execution of the thread. Thread's Thread::Run() method will
	 * be run as separate thread of execution.
	 * @param stackSize - size of the stack in bytes which should be allocated for this thread.
	 *                    If stackSize is 0 then system default stack size is used
	 *                    (stack size depends on underlying OS).
	 */
	void start(size_t stackSize = 0);



	/**
	 * @brief Wait for thread to finish its execution.
	 * This function waits for the thread finishes its execution,
	 * i.e. until the thread returns from its Thread::Run() method.
	 * Note: it is safe to call Join() on not started threads,
	 *       in that case it will return immediately.
	 */
	void join()noexcept;



	/**
	 * @brief Suspend the thread for a given number of milliseconds.
	 * Suspends the thread which called this function for a given number of milliseconds.
	 * This function guarantees that the calling thread will be suspended for
	 * AT LEAST 'msec' milliseconds.
	 * @param msec - number of milliseconds the thread should be suspended.
	 */
	static void sleep(unsigned msec = 0)noexcept{
#if M_OS == M_OS_WINDOWS
		SleepEx(DWORD(msec), FALSE);// Sleep() crashes on MinGW (I do not know why), this is why SleepEx() is used here.
#elif M_OS == M_OS_SYMBIAN
		User::After(msec * 1000);
#elif M_OS == M_OS_UNIX || M_OS == M_OS_MACOSX || M_OS == M_OS_LINUX
		if(msec == 0){
#	if M_OS == M_OS_UNIX || M_OS == M_OS_MACOSX || M_OS_NAME == M_OS_NAME_ANDROID
			sched_yield();
#	elif M_OS == M_OS_LINUX
			pthread_yield();
#	else
#		error "Unsupported OS"
#	endif
		}else{
			usleep(msec * 1000);
		}
#else
#	error "Unsupported OS"
#endif
	}



	/**
	 * @brief Thread ID type.
	 * Thread ID type is used to identify a thread.
	 * The type supports operator==() and operator!=() operators.
	 */
#if M_OS == M_OS_WINDOWS
	typedef std::uint32_t T_ThreadID;
#else
	typedef unsigned long int T_ThreadID;
#endif



	/**
	 * @brief get current thread ID.
	 * Returns unique identifier of the currently executing thread. This ID can further be used
	 * to make assertions to make sure that some code is executed in a specific thread. E.g.
	 * assert that methods of some object are executed in the same thread where this object was
	 * created.
	 * @return unique thread identifier.
	 */
	static inline T_ThreadID getCurrentThreadID()noexcept{
#if M_OS == M_OS_WINDOWS
		return T_ThreadID(GetCurrentThreadId());
#elif M_OS == M_OS_MACOSX || M_OS == M_OS_LINUX
		pthread_t t = pthread_self();
		static_assert(sizeof(pthread_t) <= sizeof(T_ThreadID), "error");
		return T_ThreadID(t);
#else
#	error "Unsupported OS"
#endif
	}
};




}//~namespace
