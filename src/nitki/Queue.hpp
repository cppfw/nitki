/**
 * @author Ivan Gagis <igagis@gmail.com>
 * @author Jose Luis Hidalgo <joseluis.hidalgo@gmail.com> - Mac OS X port
 */

#pragma once

#include <utki/config.hpp>
#include <utki/debug.hpp>
#include <utki/SpinLock.hpp>

#include <pogodi/WaitSet.hpp>

#include <list>
#include <functional>


namespace nitki{



/**
 * @brief Message queue.
 * Message queue is used for communication of separate threads by
 * means of sending messages to each other. Thus, when one thread sends a message to another one,
 * it asks that another thread to execute some code portion - handler code of the message.
 * NOTE: Queue implements Waitable interface which means that it can be used in conjunction
 * with pogodi::WaitSet. But, note, that the implementation of the Waitable is that it
 * shall only be used to wait for READ. If you are trying to wait for WRITE the behavior will be
 * undefined.
 */
class DLLEXPORT Queue : public pogodi::Waitable{
public:
	typedef std::function<void()> T_Message;
	
private:
	utki::SpinLock mut;

	std::list<T_Message> messages;
	
#if M_OS == M_OS_WINDOWS
	//use Event to implement Waitable on Windows
	HANDLE eventForWaitable;
#elif M_OS == M_OS_MACOSX
	//use pipe to implement Waitable in *nix systems
	int pipeEnds[2];
#elif M_OS == M_OS_LINUX
	//use eventfd()
	int eventFD;
#else
#	error "Unsupported OS"
#endif

	//forbid copying
	Queue(const Queue&);
	Queue& operator=(const Queue&);

public:
	/**
	 * @brief Constructor, creates empty message queue.
	 */
	Queue();

	
	/**
	 * @brief Destructor.
	 * When called, it also destroys all messages on the queue.
	 */
	~Queue()noexcept;



	/**
	 * @brief Pushes a new message to the queue.
	 * @param msg - the message to push into the queue.
	 */
	void pushMessage(T_Message&& msg)noexcept;



	/**
	 * @brief Get message from queue, does not block if no messages queued.
	 * This method gets a message from message queue. If there are no messages on the queue
	 * it will return invalid auto pointer.
	 * @return auto-pointer to Message instance.
	 * @return invalid auto-pointer if there are no messages in the queue.
	 */
	T_Message peekMsg();


private:
#if M_OS == M_OS_WINDOWS
	HANDLE getHandle()override;

	std::uint32_t flagsMask;//flags to wait for

	void setWaitingEvents(std::uint32_t flagsToWaitFor)override;

	//returns true if signaled
	bool checkSignaled()override;

#elif M_OS == M_OS_LINUX
	int getHandle()override;

#elif M_OS == M_OS_MACOSX
	int getHandle()override;

#else
#	error "Unsupported OS"
#endif
};



}//~namespace
