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

} // namespace Utility

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/thread/condition.hpp>

#ifdef NDEBUG
void boost::throw_exception(std::exception const&)
{
}
#endif