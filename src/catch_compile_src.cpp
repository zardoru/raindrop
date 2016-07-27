#include "pch.h"
#include "Logging.h"

#define CATCH_CONFIG_RUNNER
#include "ext/catch.hpp"

void RunRaindropTests()
{
	Catch::Session session;

	Log::Printf("Return value after running tests: %d.\n", session.run());

	std::cin.get();
}