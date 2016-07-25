#include "pch.h"

#include "Logging.h"

using std::wcout;
static std::fstream logfile ("log.txt", std::ios::out);

#ifndef NDEBUG
void Log::DebugPrintf(std::string Format, ...)
{
    LogPrintf(Format);
}
#else
void Log::DebugPrintf(std::string Format, ...)
{
    // stub
}
#endif

void Log::Printf(std::string Format, ...)
{
    char Buffer[2048];
    va_list vl;
    va_start(vl, Format);
    vsnprintf(Buffer, 2048, Format.c_str(), vl);
    va_end(vl);
	wprintf(L"%hs", Buffer);
}

void Log::Logf(std::string Format, ...)
{
    char Buffer[2048];
    va_list vl;
    va_start(vl, Format);
    vsnprintf(Buffer, 2048, Format.c_str(), vl);
    va_end(vl);
    logfile << Buffer;
    logfile.flush();
}

void Log::LogPrintf(std::string str, ...)
{
    char Buffer[2048];
    va_list vl;
    va_start(vl, str);
    vsnprintf(Buffer, 2048, str.c_str(), vl);
    va_end(vl);
	wprintf(L"%hs", Buffer);
    logfile << Buffer;
	logfile.flush();
}