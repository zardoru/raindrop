#ifndef LOGGING_H_
#define LOGGING_H_

namespace Log
{
#ifdef DEBUG
	static void DebugPrintf(String Format, ...);
#else
#define DebugPrintf(...)
#endif

	void Printf(String Format, ...);
	void Logf(String Format, ...);
};

#endif