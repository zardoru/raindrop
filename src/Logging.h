#ifndef LOGGING_H_
#define LOGGING_H_

namespace Log
{
#ifdef DEBUG
	static void DebugPrintf(GString Format, ...);
#else
#define DebugPrintf(...)
#endif

	void Printf(GString Format, ...);
	void Logf(GString Format, ...);
};

#endif