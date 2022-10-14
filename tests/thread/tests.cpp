#include <utki/debug.hpp>
#include <utki/config.hpp>
#include <utki/span.hpp>

#include <opros/wait_set.hpp>

#include "../../src/nitki/thread.hpp"
#include "../../src/nitki/queue.hpp"

#include "tests.hpp"



namespace TestJoinBeforeAndAfterThreadHasFinished{

class TestThread : public nitki::thread{
public:
	int a, b;

	void run()override{
		this->a = 10;
		this->b = 20;
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		this->a = this->b;
	}
};



void Run(){
	//Test join after thread has finished
	{
		TestThread t;

		t.start();

		std::this_thread::sleep_for(std::chrono::milliseconds(2000));

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

class TestThread1 : public nitki::thread{
public:
	TestThread1(){}

	nitki::queue queue;
	volatile bool quitFlag = false;

	int a, b;

	void run()override{
		opros::wait_set ws(1);
		
		ws.add(this->queue, {opros::ready::read});
		
		while(!this->quitFlag){
			ws.wait();
			while(auto m = this->queue.pop_front()){
				m();
			}
		}
		
		ws.remove(this->queue);
	}
};



void Run(){
	//TODO: read ulimit
	size_t num_threads =
#if M_OS == M_OS_MACOSX
			50
#else
			500
#endif
	;

	std::vector<std::unique_ptr<TestThread1>> thr;

	for(size_t i = 0; i != num_threads; ++i){
		auto t = std::make_unique<TestThread1>();

		try{
			t->start();
		}catch(std::system_error& e){
			utki::log([&](auto& o){
				o << "exception caught during thread creation: " << e.what() << ",\n";
				o << "continuing to stopping already created threads" << '\n';
			});
			break;
		}

		thr.push_back(std::move(t));
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(1000));

	for(auto i = thr.begin(); i != thr.end(); ++i){
		(*i)->quitFlag = true;
		(*i)->queue.push_back([](){});
		(*i)->join();
	}
}

}



//==========================
//Test immediate thread exit
//==========================

namespace TestImmediateExitThread{

class ImmediateExitThread : public nitki::thread{
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

class TestRunnerThread : public nitki::thread{
public:
	class TopLevelThread : public nitki::thread{
	public:

		class InnerLevelThread : public nitki::thread{
		public:

			void run()override{
			}
		} inner;

		void run()override{
			this->inner.start();
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
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

	std::this_thread::sleep_for(std::chrono::milliseconds(1000));

	utki::assert(runner.success, SL);

	runner.join();
}


}//~namespace
