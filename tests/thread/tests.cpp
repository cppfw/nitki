#include <utki/debug.hpp>
#include <utki/config.hpp>
#include <utki/span.hpp>

#include <opros/wait_set.hpp>

#include "../../src/nitki/thread.hpp"
#include "../../src/nitki/queue.hpp"

#include "tests.hpp"

#ifdef assert
#	undef assert
#endif

namespace test_join_before_and_after_thread_has_finished{

class test_thread : public nitki::thread{
public:
	int a, b;

	void run()override{
		this->a = 10;
		this->b = 20;
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		this->a = this->b;
	}
};



void run(){
	//Test join after thread has finished
	{
		test_thread t;

		t.start();

		std::this_thread::sleep_for(std::chrono::milliseconds(2000));

		t.join();
	}



	//Test join before thread has finished
	{
		test_thread t;

		t.start();

		t.join();
	}
}

}


//===================
// Test many threads
//===================
namespace test_many_threads{

class test_thread_1 : public nitki::thread{
public:
	test_thread_1() = default;

	nitki::queue queue;
	volatile bool quit_flag = false;

	int a, b;

	void run()override{
		opros::wait_set ws(1);
		
		ws.add(this->queue, {opros::ready::read});
		
		while(!this->quit_flag){
			ws.wait(nullptr);
			while(auto m = this->queue.pop_front()){
				m();
			}
		}
		
		ws.remove(this->queue);
	}
};

void run(){
	// TODO: read ulimit
	size_t num_threads =
#if CFG_OS == CFG_OS_MACOSX
			50
#else
			500
#endif
	;

	std::vector<std::unique_ptr<test_thread_1>> thr;

	for(size_t i = 0; i != num_threads; ++i){
		auto t = std::make_unique<test_thread_1>();

		try{
			ASSERT(t)
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

	for(auto& t : thr){
		t->quit_flag = true;
		t->queue.push_back([](){});
		t->join();
	}
}

}



//==========================
//Test immediate thread exit
//==========================

namespace test_immediate_exit_thread{

class immediate_exit_thread : public nitki::thread{
public:
	void run()override{
		return;
	}
};


void run(){
	for(unsigned i = 0; i < 100; ++i){
		immediate_exit_thread t;
		t.start();
		t.join();
	}
}

}



namespace test_nested_join{

class test_runner_thread : public nitki::thread{
public:
	class top_level_thread : public nitki::thread{
	public:

		class inner_level_thread : public nitki::thread{
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

	volatile bool success = false;

	test_runner_thread() = default;

	void run()override{
		this->top.start();
		this->top.join();
		this->success = true;
	}
};



void run(){
	test_runner_thread runner;
	runner.start();

	std::this_thread::sleep_for(std::chrono::milliseconds(1000));

	utki::assert(runner.success, SL);

	runner.join();
}


}//~namespace
