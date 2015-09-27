#include "Thread.hpp"


#if M_OS == M_OS_WINDOWS
#	include <process.h>
#endif

#include <cstring>


using namespace nitki;



namespace{



std::mutex threadMutex2;



}//~namespace



//Tread Run function
//static
#if M_OS == M_OS_WINDOWS
unsigned int __stdcall Thread::runThread(void *data)
#elif M_OS == M_OS_SYMBIAN
TInt Thread::runThread(TAny *data)
#elif M_OS == M_OS_LINUX || M_OS == M_OS_MACOSX
void* Thread::runThread(void *data)
#else
#	error "Unsupported OS"
#endif
{
	Thread *thr = reinterpret_cast<Thread*>(data);
	try{
		thr->run();
	}catch(utki::Exc& DEBUG_CODE(e)){
		ASSERT_INFO(false, "uncaught utki::Exc exception in Thread::Run(): " << e.What())
	}catch(std::exception& DEBUG_CODE(e)){
		ASSERT_INFO(false, "uncaught std::exception exception in Thread::Run(): " << e.what())
	}catch(...){
		ASSERT_INFO(false, "uncaught unknown exception in Thread::Run()")
	}

	{
		//protect by mutex to avoid changing the
		//this->state variable before Start() has finished.
		std::lock_guard<decltype(threadMutex2)> mutexGuard(threadMutex2);

		thr->state = E_State::STOPPED;
	}

#if M_OS == M_OS_WINDOWS
	//Do nothing, _endthreadex() will be called   automatically
	//upon returning from the thread routine.
#elif M_OS == M_OS_LINUX || M_OS == M_OS_MACOSX
	pthread_exit(0);
#else
#	error "Unsupported OS"
#endif
	return 0;
}



Thread::Thread(){
#if M_OS == M_OS_WINDOWS
	this->th = NULL;
#elif M_OS == M_OS_SYMBIAN
	//do nothing
#elif M_OS == M_OS_LINUX || M_OS == M_OS_MACOSX
	//do nothing
#else
#	error "Unsuported OS"
#endif
}



Thread::~Thread()noexcept{
	ASSERT_INFO(
			this->state == JOINED || this->state == NEW,
			"~Thread() destructor is called while the thread was not joined before. "
			<< "Make sure the thread is joined by calling Thread::Join() "
			<< "before destroying the thread object."
		)

	//NOTE: it is incorrect to put this->Join() to this destructor, because
	//thread shall already be stopped at the moment when this destructor
	//is called. If it is not, then the thread will be still running
	//when part of the thread object is already destroyed, since thread object is
	//usually a derived object from Thread class and the destructor of this derived
	//object will be called before ~Thread() destructor.
}



void Thread::start(size_t stackSize){
	//Protect by mutex to avoid several Start() methods to be called
	//by concurrent threads simultaneously and to protect call to Join() before Start()
	//has returned.
	std::lock_guard<decltype(this->mutex1)> mutexGuard1(this->mutex1);
	
	//Protect by mutex to avoid incorrect state changing in case when thread
	//exits before the Start() method returned.
	std::lock_guard<decltype(threadMutex2)> mutexGuard2(threadMutex2);

	if(this->state != E_State::NEW){
		throw ThreadHasAlreadyBeenStartedExc();
	}

#if M_OS == M_OS_WINDOWS
	this->th = reinterpret_cast<HANDLE>(
			_beginthreadex(
					NULL,
					stackSize > size_t(unsigned(-1)) ? unsigned(-1) : unsigned(stackSize),
					&runThread,
					reinterpret_cast<void*>(this),
					0,
					NULL
				)
		);
	if(this->th == NULL){
		throw Exc("Thread::Start(): _beginthreadex failed");
	}
#elif M_OS == M_OS_SYMBIAN
	if(this->th.Create(_L("ting thread"), &runThread,
				stackSize == 0 ? KDefaultStackSize : stackSize,
				NULL, reinterpret_cast<TAny*>(this)) != KErrNone
			)
	{
		throw Exc("Thread::Start(): starting thread failed");
	}
	this->th.Resume();//start the thread execution
#elif M_OS == M_OS_LINUX || M_OS == M_OS_MACOSX
	{
		pthread_attr_t attr;

		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
		pthread_attr_setstacksize(&attr, stackSize);

		int res = pthread_create(&this->th, &attr, &runThread, this);
		if(res != 0){
			pthread_attr_destroy(&attr);
			TRACE_AND_LOG(<< "Thread::Start(): pthread_create() failed, error code = " << res
					<< " meaning: " << strerror(res) << std::endl)
			std::stringstream ss;
			ss << "Thread::Start(): starting thread failed,"
					<< " error code = " << res << ": " << strerror(res);
			throw Exc(ss.str());
		}
		pthread_attr_destroy(&attr);
	}
#else
#	error "Unsupported OS"
#endif
	this->state = E_State::RUNNING;
}



void Thread::join() noexcept{
//	TRACE(<< "Thread::Join(): enter" << std::endl)

	//protect by mutex to avoid several Join() methods to be called by concurrent threads simultaneously.
	//NOTE: excerpt from pthread docs: "If multiple threads simultaneously try to join with the same thread, the results are undefined."
	std::lock_guard<decltype(this->mutex1)> mutexGuard(this->mutex1);

	if(this->state == E_State::NEW){
		//thread was not started, do nothing
		return;
	}

	if(this->state == E_State::JOINED){
		return;
	}

	ASSERT(this->state == E_State::RUNNING || this->state == E_State::STOPPED)
	
#if M_OS == M_OS_WINDOWS
	ASSERT_INFO(this->th != GetCurrentThread(), "tried to call Join() on the current thread")
#else
	ASSERT_INFO(T_ThreadID(this->th) != nitki::Thread::getCurrentThreadID(), "tried to call Join() on the current thread")
#endif

#if M_OS == M_OS_WINDOWS
	WaitForSingleObject(this->th, INFINITE);
	CloseHandle(this->th);
	this->th = NULL;
#elif M_OS == M_OS_SYMBIAN
	TRequestStatus reqStat;
	this->th.Logon(reqStat);
	User::WaitForRequest(reqStat);
	this->th.Close();
#elif M_OS == M_OS_LINUX || M_OS == M_OS_MACOSX
#	ifdef DEBUG
	int res =
#	endif
			pthread_join(this->th, 0);
	ASSERT_INFO(res == 0, "res = " << strerror(res))
#else
#	error "Unsupported OS"
#endif

	//NOTE: at this point the thread's Run() method should already exit and state
	//should be set to STOPPED
	ASSERT_INFO(this->state == E_State::STOPPED, "this->state = " << this->state)

	this->state = E_State::JOINED;

//	TRACE(<< "Thread::Join(): exit" << std::endl)
}
