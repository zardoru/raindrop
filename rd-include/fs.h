#if _MSC_VER >= 1900
	#include <filesystem>
	namespace std
	{
		namespace filesystem = experimental::filesystem;
	}
	#define STD_FILESYSTEM
#else

    #ifdef __clang__
        #include <experimental/filesystem>
	    namespace std
	    {
		    namespace filesystem = experimental::filesystem;
	    }
	    #define STD_FILESYSTEM
    #else

	    #ifndef __GNUC__
		    #include <boost/filesystem.hpp>
		    namespace std
		    {
			    namespace filesystem = boost::filesystem;
		    }
	    #else // it's GCC
			#include <features.h>
		    #if __GNUC_PREREQ(6,1)
			    // We can alias the filesystem
			    #include <experimental/filesystem>
			    namespace std
			    {
				    namespace filesystem = experimental::filesystem;
			    }
			    #define STD_FILESYSTEM
		    #else
			    // okay then, use boost
			    #include <boost/filesystem.hpp>
			    namespace std
			    {
				    namespace filesystem = boost::filesystem;
			    }
			    #endif
	    #endif

    #endif
#endif



#ifdef _WIN32
#define CreateIfstream(name, fn) std::ifstream name(fn.wstring());
#define CreateBinIfstream(name, fn) std::fstream name(fn.wstring(), std::ios::in | std::ios::binary);
#define CreateOfstream(name, fn) std::ofstream name(fn.wstring());
#define CreateBinOfstream(name, fn) std::ofstream name(fn.wstring(), std::ios::binary);
#else
#define CreateIfstream(name, fn) std::ifstream name(fn.string());
#define CreateOfstream(name, fn) std::ofstream name(fn.string());
#define CreateBinIfstream(name, fn) std::fstream name(fn.string(), std::ios::in | std::ios::binary);
#define CreateBinOfstream(name, fn) std::ofstream name(fn.string(), std::ios::binary);
#endif

// template <class T>
// void BinWrite(std::ofstream &of, T obj) {
// 	of.write((char*)&obj, sizeof(T));
// }

// template <class T>
// void BinRead(std::ifstream &of, T& obj) {
// 	of.read((char*)&obj, sizeof(T));
// }