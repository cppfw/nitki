#include "thread.hpp"

using namespace nitki;

namespace{
void run_thread(void *data){
	thread *thr = reinterpret_cast<thread*>(data);
	
	thr->run();
}
}

thread::~thread()noexcept{
	ASSERT_INFO(!this->thr.joinable(),
			"~thread() destructor is called while the thread was not joined before. "
			<< "Make sure the thread is joined by calling thread::join() "
			<< "before destroying the thread object."
		)

	// NOTE: it is incorrect to put this->join() to this destructor, because
	//       thread shall already be stopped at the moment when this destructor
	//       is called. If it is not, then the thread will be still running
	//       when part of the thread object is already destroyed, since thread object is
	//       usually a derived object from thread class and the destructor of this derived
	//       object will be called before ~thread() destructor.
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
