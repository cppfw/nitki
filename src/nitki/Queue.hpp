#pragma once

#include <utki/config.hpp>
#include <utki/debug.hpp>
#include <utki/spin_lock.hpp>

#include <opros/wait_set.hpp>

#include <list>
#include <functional>


namespace nitki{



/**
 * @brief Message queue.
 * Message queue is used for communication of separate threads by
 * means of sending messages to each other. Thus, when one thread sends a message to another one,
 * it asks that another thread to execute some code portion - handler code of the message.
 * NOTE: Queue implements waitable interface which means that it can be used in conjunction
 * with opros::wait_set. But, note, that the implementation of the waitable is that it
 * shall only be used to wait for READ. If you are trying to wait for WRITE the behavior will be
 * undefined.
 */
class Queue : public opros::waitable{
public:
	typedef std::function<void()> T_Message;
	
private:
	utki::spin_lock mut;

	std::list<T_Message> messages;
	
#if M_OS == M_OS_WINDOWS
	//use Event to implement waitable on Windows
	HANDLE eventForwaitable;
#elif M_OS == M_OS_MACOSX
	//use pipe to implement waitable in *nix systems
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


#if M_OS == M_OS_WINDOWS
protected:
	HANDLE get_handle()override;

	std::uint32_t waiting_flags;

	void set_waiting_flags(utki::flags<opros::ready> wait_for)override;

	bool check_signaled()override;

#elif M_OS == M_OS_LINUX
public:
	int get_handle()override;

#elif M_OS == M_OS_MACOSX
public:
	int get_handle()override;

#else
#	error "Unsupported OS"
#endif
};

}
