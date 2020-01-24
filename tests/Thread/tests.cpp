#include <utki/debug.hpp>
#include <utki/config.hpp>
#include <utki/Buf.hpp>

#include <opros/wait_set.hpp>

#include "../../src/nitki/Thread.hpp"
#include "../../src/nitki/MsgThread.hpp"


#include "tests.hpp"



namespace TestJoinBeforeAndAfterThreadHasFinished{

class TestThread : public nitki::Thread{
public:
	int a, b;

	void run()override{
		this->a = 10;
		this->b = 20;
		nitki::Thread::sleep(1000);
		this->a = this->b;
	}
};



void Run(){
	//Test join after thread has finished
	{
		TestThread t;

		t.start();

		nitki::Thread::sleep(2000);

		t.join();
	}



	//Test join before thread has finished
	{
		TestThread t;

		t.start();

		t.join();
	}
}

}


//===================
// Test many threads
//===================
namespace TestManyThreads{

class TestThread1 : public nitki::MsgThread{
public:
	int a, b;

	void run()override{
		opros::wait_set ws(1);
		
		ws.add(this->queue, {opros::ready::read});
		
		while(!this->quitFlag){
			ws.wait();
			while(auto m = this->queue.peekMsg()){
				m();
			}
		}
		
		ws.remove(this->queue);
	}
};



void Run(){
	//TODO: read ulimit
	std::array<
			TestThread1,
#if M_OS == M_OS_MACOSX
			50
#else
			500
#endif
		> thr;

	for(auto i = thr.begin(); i != thr.end(); ++i){
		i->start();
	}

	nitki::Thread::sleep(1000);

	for(auto i = thr.begin(); i != thr.end(); ++i){
		i->pushQuitMessage();
		i->join();
	}
}

}



//==========================
//Test immediate thread exit
//==========================

namespace TestImmediateExitThread{

class ImmediateExitThread : public nitki::Thread{
public:
	void run()override{
		return;
	}
};


void Run(){
	for(unsigned i = 0; i < 100; ++i){
		ImmediateExitThread t;
		t.start();
		t.join();
	}
}

}



namespace TestNestedJoin{

class TestRunnerThread : public nitki::Thread{
public:
	class TopLevelThread : public nitki::Thread{
	public:

		class InnerLevelThread : public nitki::Thread{
		public:

			//overrun
			void run(){
			}
		} inner;

		//override
		void run(){
			this->inner.start();
			nitki::Thread::sleep(100);
			this->inner.join();
		}
	} top;

	volatile bool success;

	TestRunnerThread() :
			success(false)
	{}

	void run()override{
		this->top.start();
		this->top.join();
		this->success = true;
	}
};



void Run(){
	TestRunnerThread runner;
	runner.start();

	nitki::Thread::sleep(1000);

	ASSERT_ALWAYS(runner.success)

	runner.join();
}


}//~namespace
