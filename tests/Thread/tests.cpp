#include "../../src/ting/debug.hpp"
#include "../../src/ting/mt/Thread.hpp"
#include "../../src/ting/mt/MsgThread.hpp"
#include "../../src/ting/Buffer.hpp"
#include "../../src/ting/types.hpp"
#include "../../src/ting/config.hpp"
#include "../../src/ting/WaitSet.hpp"

#include "tests.hpp"



namespace TestJoinBeforeAndAfterThreadHasFinished{

class TestThread : public ting::mt::Thread{
public:
	int a, b;

	//override
	void Run(){
		this->a = 10;
		this->b = 20;
		ting::mt::Thread::Sleep(1000);
		this->a = this->b;
	}
};



void Run(){

	//Test join after thread has finished
	{
		TestThread t;

		t.Start();

		ting::mt::Thread::Sleep(2000);

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

class TestThread1 : public ting::mt::MsgThread{
public:
	int a, b;

	//override
	void Run(){
		ting::WaitSet ws(1);
		
		ws.Add(this->queue, ting::Waitable::READ);
		
		while(!this->quitFlag){
			ws.Wait();
			while(auto m = this->queue.PeekMsg()){
				m();
			}
		}
		
		ws.Remove(this->queue);
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

	ting::mt::Thread::Sleep(1000);

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

class ImmediateExitThread : public ting::mt::Thread{
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

class TestRunnerThread : public ting::mt::Thread{
public:
	class TopLevelThread : public ting::mt::Thread{
	public:

		class InnerLevelThread : public ting::mt::Thread{
		public:

			//overrun
			void Run(){
			}
		} inner;

		//override
		void Run(){
			this->inner.Start();
			ting::mt::Thread::Sleep(100);
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

	ting::mt::Thread::Sleep(1000);

	ASSERT_ALWAYS(runner.success)

	runner.Join();
}


}//~namespace
