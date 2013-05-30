#include "Global.h"
#include <cstring>

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