#pragma once

#include <utki/debug.hpp>

#include "tests.hpp"



inline void TestTingThread(){
 	std::cout << "running TestManyThreads..." << std::endl;
	TestManyThreads::Run();

	std::cout << "running TestJoinBeforeAndAfterThreadHasFinished" << std::endl;
	TestJoinBeforeAndAfterThreadHasFinished::Run();

	std::cout << "running TestImmediateExitThread" << std::endl;
	TestImmediateExitThread::Run();

	std::cout << "running TestNestedJoin" << std::endl;
	TestNestedJoin::Run();
}
