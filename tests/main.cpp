#include <filesystem>
#include <catch2/catch_session.hpp>


int main(int argc, char const *argv[])
{
    Catch::Session session;
	auto res = session.run(argc, argv);
	//std::cout << "cwd: " << std::filesystem::current_path() << std::endl;

    return res;
}
