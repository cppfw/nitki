#include <nitki/queue.hpp>

int main(int argc, const char** argv){
	nitki::queue q;

	q.push_back([](){
		std::cout << "Hello nitki!" << '\n';
	});

	auto f = q.pop_front();
	f();
}
