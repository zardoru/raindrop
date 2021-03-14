#define CATCH_CONFIG_RUNNER
#include <filesystem>
#include <catch.hpp>


int main(int argc, char const *argv[])
{
    Catch::Session session;
	session.applyCommandLine(argc, argv);
	std::cout << "cwd: " << std::filesystem::current_path() << std::endl;

    return session.run();
}
