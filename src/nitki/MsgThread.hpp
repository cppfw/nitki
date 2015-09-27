
/**
 * @author Ivan Gagis <igagis@gmail.com>
 * @author Jose Luis Hidalgo <joseluis.hidalgo@gmail.com> - Mac OS X port
 */

#pragma once

#include "Thread.hpp"
#include "Queue.hpp"



namespace nitki{



class QuitMessage;



/**
 * @brief a thread with message queue.
 * This is just a facility class which already contains message queue and boolean 'quit' flag.
 */
class MsgThread : public Thread{
protected:
	/**
	 * @brief Flag indicating that the thread should exit.
	 * This is a flag used to stop thread execution. The implementor of
	 * Thread::Run() method usually would want to use this flag as indicator
	 * of thread exit request. If this flag is set to true then the thread is requested to exit.
	 * The typical usage of the flag is as follows:
	 * @code
	 * class MyThread : public ting::MsgThread{
	 *     ...
	 *     void MyThread::Run(){
	 *         while(!this->quitFlag){
	 *             //get and handle thread messages, etc.
	 *             ...
	 *         }
	 *     }
	 *     ...
	 * };
	 * @endcode
	 */
	volatile bool quitFlag = false;//looks like it is not necessary to protect this flag by mutex, volatile will be enough

	Queue queue;

	Queue::T_Message quitMessage = [this](){this->quitFlag = true;};

public:
	MsgThread() = default;

	~MsgThread()noexcept{}
	
	/**
	 * @brief Send preallocated 'Quit' message to thread's queue.
	 * This function throws no exceptions. It can send the quit message only once.
	 */
	void PushPreallocatedQuitMessage()noexcept;
	
	
	
	/**
	 * @brief Send 'Quit' message to thread's queue.
	 */
	void PushQuitMessage(){
		this->PushMessage([this](){this->quitFlag = true;});
	}



	/**
	 * @brief Send "no operation" message to thread's queue.
	 */
	void PushNopMessage(){
		this->PushMessage([](){});
	}



	/**
	 * @brief Send a message to thread's queue.
	 * @param msg - a message to send.
	 */
	void PushMessage(Queue::T_Message&& msg)noexcept{
		this->queue.PushMessage(std::move(msg));
	}
};



}//~namespace
