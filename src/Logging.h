#pragma once

namespace Log
{
	void DebugPrintf(GString Format, ...);

	void Printf(GString Format, ...);
	void Logf(GString Format, ...);
	void LogPrintf(GString str, ...);
};