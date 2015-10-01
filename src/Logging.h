#ifndef LOGGING_H_
#define LOGGING_H_

namespace Log
{
	void DebugPrintf(GString Format, ...);

	void Printf(GString Format, ...);
	void Logf(GString Format, ...);
	void LogPrintf(GString str, ...);
};

#endif