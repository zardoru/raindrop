#define CATCH_CONFIG_RUNNER
#include "ext/catch.hpp"


int main(int argc, char const *argv[])
{
    Catch::Session session;
	session.applyCommandLine(argc, argv);

	std::cout << session.run();

	std::cin.get();
    return 0;
}
