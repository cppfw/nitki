#include "Queue.hpp"

#include <mutex>

#if M_OS == M_OS_LINUX
#	include <sys/eventfd.h>
#	include <cstring>
#endif


//#define M_ENABLE_QUEUE_TRACE
#ifdef M_ENABLE_QUEUE_TRACE
#	define M_QUEUE_TRACE(x) TRACE(<< "[QUEUE] ") TRACE(x)
#else
#	define M_QUEUE_TRACE(x)
#endif




using namespace nitki;



Queue::Queue(){
	//can write will always be set because it is always possible to post a message to the queue
	this->setCanWriteFlag();

#if M_OS == M_OS_WINDOWS
	this->eventForWaitable = CreateEvent(
			NULL, //security attributes
			TRUE, //manual-reset
			FALSE, //not signalled initially
			NULL //no name
		);
	if(this->eventForWaitable == NULL){
		throw utki::Exc("Queue::Queue(): could not create event (Win32) for implementing Waitable");
	}
#elif M_OS == M_OS_MACOSX
	if(::pipe(&this->pipeEnds[0]) < 0){
		std::stringstream ss;
		ss << "Queue::Queue(): could not create pipe (*nix) for implementing Waitable,"
				<< " error code = " << errno << ": " << strerror(errno);
		TRACE(<< ss.str() << std::endl)
		throw utki::Exc(ss.str().c_str());
	}
#elif M_OS == M_OS_LINUX
	this->eventFD = eventfd(0, EFD_NONBLOCK);
	if(this->eventFD < 0){
		std::stringstream ss;
		ss << "Queue::Queue(): could not create eventfd (linux) for implementing Waitable,"
				<< " error code = " << errno << ": " << strerror(errno);
		throw utki::Exc(ss.str().c_str());
	}
#else
#	error "Unsupported OS"
#endif
}



Queue::~Queue()noexcept{
#if M_OS == M_OS_WINDOWS
	CloseHandle(this->eventForWaitable);
#elif M_OS == M_OS_MACOSX
	close(this->pipeEnds[0]);
	close(this->pipeEnds[1]);
#elif M_OS == M_OS_LINUX
	close(this->eventFD);
#else
#	error "Unsupported OS"
#endif
}



void Queue::PushMessage(std::function<void()>&& msg)noexcept{
	std::lock_guard<decltype(this->mut)> mutexGuard(this->mut);
	this->messages.push_back(std::move(msg));
	
	if(this->messages.size() == 1){//if it is a first message
		//Set CanRead flag.
		//NOTE: in linux implementation with epoll(), the CanRead
		//flag will also be set in WaitSet::Wait() method.
		//NOTE: set CanRead flag before event notification/pipe write, because
		//if do it after then some other thread which was waiting on the WaitSet
		//may read the CanRead flag while it was not set yet.
		ASSERT(!this->CanRead())
		this->setCanReadFlag();

#if M_OS == M_OS_WINDOWS
		if(SetEvent(this->eventForWaitable) == 0){
			ASSERT(false)
		}
#elif M_OS == M_OS_MACOSX
		{
			std::uint8_t oneByteBuf[1];
			if(write(this->pipeEnds[1], oneByteBuf, 1) != 1){
				ASSERT(false)
			}
		}
#elif M_OS == M_OS_LINUX
		if(eventfd_write(this->eventFD, 1) < 0){
			ASSERT(false)
		}
#else
#	error "Unsupported OS"
#endif
	}

	ASSERT(this->CanRead())
}



Queue::T_Message Queue::PeekMsg(){
	std::lock_guard<decltype(this->mut)> mutexGuard(this->mut);
	if(this->messages.size() != 0){
		ASSERT(this->CanRead())

		if(this->messages.size() == 1){//if we are taking away the last message from the queue
#if M_OS == M_OS_WINDOWS
			if(ResetEvent(this->eventForWaitable) == 0){
				ASSERT(false)
				throw utki::Exc("Queue::Wait(): ResetEvent() failed");
			}
#elif M_OS == M_OS_MACOSX
			{
				std::uint8_t oneByteBuf[1];
				if(read(this->pipeEnds[0], oneByteBuf, 1) != 1){
					throw utki::Exc("Queue::Wait(): read() failed");
				}
			}
#elif M_OS == M_OS_LINUX
			{
				eventfd_t value;
				if(eventfd_read(this->eventFD, &value) < 0){
					throw utki::Exc("Queue::Wait(): eventfd_read() failed");
				}
				ASSERT(value == 1)
			}
#else
#	error "Unsupported OS"
#endif
			this->clearCanReadFlag();
		}else{
			ASSERT(this->canRead())
		}
		
		T_Message ret = std::move(this->messages.front());
		
		this->messages.pop_front();
		
		return std::move(ret);
	}
	return nullptr;
}



#if M_OS == M_OS_WINDOWS
//override
HANDLE Queue::GetHandle(){
	//return event handle
	return this->eventForWaitable;
}



//override
void Queue::SetWaitingEvents(std::uint32_t flagsToWaitFor){
	//It is not allowed to wait on queue for write,
	//because it is always possible to push new message to queue.
	//Error condition is not possible for Queue.
	//Thus, only possible flag values are READ and 0 (NOT_READY)
	if(flagsToWaitFor != 0 && flagsToWaitFor != pogodi::Waitable::READ){
		ASSERT_INFO(false, "flagsToWaitFor = " << flagsToWaitFor)
		throw utki::Exc("Queue::SetWaitingEvents(): flagsToWaitFor should be pogodi::Waitable::READ or 0, other values are not allowed");
	}

	this->flagsMask = flagsToWaitFor;
}



//returns true if signaled
//override
bool Queue::CheckSignaled(){
	//error condition is not possible for queue
	ASSERT((this->readinessFlags & pogodi::Waitable::ERROR_CONDITION) == 0)

/*
#ifdef DEBUG
	{
		atomic::SpinLock::GuardYield mutexGuard(this->mut);
		if(this->first){
			ASSERT_ALWAYS(this->CanRead())

			//event should be in signalled state
			ASSERT_ALWAYS(WaitForSingleObject(this->eventForWaitable, 0) == WAIT_OBJECT_0)
		}

		if(this->CanRead()){
			ASSERT_ALWAYS(this->first)

			//event should be in signalled state
			ASSERT_ALWAYS(WaitForSingleObject(this->eventForWaitable, 0) == WAIT_OBJECT_0)
		}

		//if event is in signalled state
		if(WaitForSingleObject(this->eventForWaitable, 0) == WAIT_OBJECT_0){
			ASSERT_ALWAYS(this->CanRead())
			ASSERT_ALWAYS(this->first)
		}
	}
#endif
*/

	return (this->readinessFlags & this->flagsMask) != 0;
}

#elif M_OS == M_OS_MACOSX
//override
int Queue::GetHandle(){
	//return read end of pipe
	return this->pipeEnds[0];
}

#elif M_OS == M_OS_LINUX
//override
int Queue::getHandle(){
	return this->eventFD;
}

#else
#	error "Unsupported OS"
#endif
