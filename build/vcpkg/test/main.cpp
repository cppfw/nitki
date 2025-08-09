#include <iostream>

#include <nitki/queue.hpp>

int main(int argc, const char** argv){
    nitki::queue queue;

    queue.push_back([](){
        std::cout << "Hello nitki!" << std::endl;
    });

    std::cout << "queue.size() = " << queue.size() << std::endl;

    queue.pop_front()();

    return 0;
}
