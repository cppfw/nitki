#include "MsgThread.hpp"

#include <mutex>


using namespace nitki;



namespace{

std::mutex quitMessageMutex;

}



void MsgThread::pushPreallocatedQuitMessage()noexcept{
	std::lock_guard<decltype(quitMessageMutex)> mutexGuard(quitMessageMutex);
	
	if(!this->quitMessage){
		return;
	}
	
	this->queue.pushMessage(std::move(this->quitMessage));
}

