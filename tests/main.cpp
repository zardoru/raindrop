#define CATCH_CONFIG_RUNNER
#include "../src-thirdparty/catch.hpp"


int main(int argc, char const *argv[])
{
    Catch::Session session;
	session.applyCommandLine(argc, argv);

    return session.run();
}
