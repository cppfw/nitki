#include "thread.hpp"

using namespace nitki;

namespace{
void run_thread(void *data){
	thread *thr = reinterpret_cast<thread*>(data);
	
	thr->run();
}
}

void thread::start(){
	if(this->thr.joinable()){
		throw std::logic_error("thread::start(): thread is already started");
	}

	this->thr = std::thread(&run_thread, this);
}

void thread::join()noexcept{
	this->thr.join();
}
