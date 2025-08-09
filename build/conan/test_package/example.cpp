#include <iostream>

#include <nitki/loop_thread.hpp>

class my_thread : public nitki::loop_thread{
public:
	my_thread() : loop_thread(0){
		this->push_back([](){});
	}

	int a, b;

	std::optional<uint32_t> on_loop()override{
		return {};
	}
};

int main(int argc, const char** argv){

	auto t = std::make_unique<my_thread>();
	t->start();

	std::this_thread::sleep_for(std::chrono::milliseconds(1000));

	t->quit();
	t->join();

	std::cout << "\tPASSED: hello nitki!" << std::endl;

	return 0;
}
