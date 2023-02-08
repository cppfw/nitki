#pragma once

#include <utki/debug.hpp>

#include "tests.hpp"



inline void test_nitki_thread(){
 	std::cout << "running test_many_threads..." << std::endl;
	test_many_threads::run();

	std::cout << "running test_join_before_and_after_thread_has_finished" << std::endl;
	test_join_before_and_after_thread_has_finished::run();

	std::cout << "running test_immediate_exit_thread" << std::endl;
	test_immediate_exit_thread::run();

	std::cout << "running test_nested_join" << std::endl;
	test_nested_join::run();
}
