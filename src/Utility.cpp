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
		if (*s != '-' && !isdigit(*s))
			return false;
		else
			s++;

		while (*s != 0)
		{
			if (!isdigit(*s))
				return false;
			s++;
		}

		return true;
	}

	String GetExtension(String Filename)
	{
		return Filename.substr(Filename.find_last_of(".")+1);
	}

	String RelativeToPath(String Filename)
	{
		return Filename.substr(Filename.find_last_of("/")+1);
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