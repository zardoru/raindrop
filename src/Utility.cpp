#include "Global.h"
#include <cstring>

int InfinityMask = 0x7F800000;
float *PInfinity = (float*)&InfinityMask;

namespace Utility {

	void DebugBreak()
	{
#ifndef NDEBUG
#ifdef WIN32
		__asm int 3
#else
		asm("int 3");
#endif
#endif
	}

#ifndef isdigit
	bool isdigit(char s)
	{
		switch(s)
		{
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case '0':
			return true;
		default:
			return false;
		}
	}
#endif

	bool IsNumeric(const char* s)
	{
		// check for first character being a minus sign
		std::stringstream k;
		double d;
		k << s;
		k >> d;
		return !k.fail();
	}

	String GetExtension(String Filename)
	{
		return Filename.substr(Filename.find_last_of(".")+1);
	}

	String RelativeToPath(String Filename)
	{
		return Filename.substr(Filename.find_last_of("/")+1);
	}

	String RemoveExtension(String Fn)
	{
		return Fn.substr(0, Fn.find_last_of("."));
	}
} // namespace Utility

#ifdef NDEBUG
namespace boost
{
void throw_exception(std::exception const&)
{
}
}
#endif