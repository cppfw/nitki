#include <utki/debug.hpp>

#include <nitki/loop_thread.hpp>

class my_thread : public nitki::loop_thread{
public:
	my_thread() : loop_thread(0){
		this->push_back([](){});
	}

	int a, b;

	std::optional<uint32_t> on_loop()override{
		auto triggered = this->wait_set.get_triggered();
		if(!triggered.empty()){
			// only internal loop_thread::queue is added to the wait_set
			utki::assert(triggered.size() == 1, SL);

			// user_data for the loop_thread::queue must be nullptr
			utki::assert(triggered[0].user_data == nullptr, SL);
		}
		return {};
	}
};

int main(int argc, const char** argv){

	auto t = std::make_unique<my_thread>();
	t->start();

	std::this_thread::sleep_for(std::chrono::milliseconds(1000));

	t->quit();
	t->join();
}
