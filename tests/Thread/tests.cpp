#include <utki/debug.hpp>
#include <utki/config.hpp>
#include <utki/Buf.hpp>

#include <pogodi/WaitSet.hpp>

#include "../../src/nitki/Thread.hpp"
#include "../../src/nitki/MsgThread.hpp"


#include "tests.hpp"



namespace TestJoinBeforeAndAfterThreadHasFinished{

class TestThread : public nitki::Thread{
public:
	int a, b;

	//override
	void Run(){
		this->a = 10;
		this->b = 20;
		nitki::Thread::Sleep(1000);
		this->a = this->b;
	}
};



void Run(){

	//Test join after thread has finished
	{
		TestThread t;

		t.Start();

		nitki::Thread::Sleep(2000);

		t.Join();
	}



	//Test join before thread has finished
	{
		TestThread t;

		t.Start();

		t.Join();
	}
}

}//~namespace


//====================
//Test many threads
//====================
namespace TestManyThreads{

class TestThread1 : public nitki::MsgThread{
public:
	int a, b;

	//override
	void Run(){
		pogodi::WaitSet ws(1);
		
		ws.add(this->queue, pogodi::Waitable::READ);
		
		while(!this->quitFlag){
			ws.wait();
			while(auto m = this->queue.PeekMsg()){
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

	for(TestThread1 *i = thr.begin(); i != thr.end(); ++i){
		i->Start();
	}

	nitki::Thread::Sleep(1000);

	for(TestThread1 *i = thr.begin(); i != thr.end(); ++i){
		i->PushQuitMessage();
		i->Join();
	}
}

}//~namespace



//==========================
//Test immediate thread exit
//==========================

namespace TestImmediateExitThread{

class ImmediateExitThread : public nitki::Thread{
public:

	//override
	void Run(){
		return;
	}
};


void Run(){
	for(unsigned i = 0; i < 100; ++i){
		ImmediateExitThread t;
		t.Start();
		t.Join();
	}
}

}//~namespace



namespace TestNestedJoin{

class TestRunnerThread : public nitki::Thread{
public:
	class TopLevelThread : public nitki::Thread{
	public:

		class InnerLevelThread : public nitki::Thread{
		public:

			//overrun
			void Run(){
			}
		} inner;

		//override
		void Run(){
			this->inner.Start();
			nitki::Thread::Sleep(100);
			this->inner.Join();
		}
	} top;

	volatile bool success;

	TestRunnerThread() :
			success(false)
	{}

	//override
	void Run(){
		this->top.Start();
		this->top.Join();
		this->success = true;
	}
};



void Run(){
	TestRunnerThread runner;
	runner.Start();

	nitki::Thread::Sleep(1000);

	ASSERT_ALWAYS(runner.success)

	runner.Join();
}


}//~namespace
