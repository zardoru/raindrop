#include "pch.h"

#include "Logging.h"

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
    wprintf(L"%ls", Utility::Widen(Buffer).c_str());
}

void Log::Logf(std::string Format, ...)
{
    static std::fstream logf("log.txt", std::ios::out);
    char Buffer[2048];
    va_list vl;
    va_start(vl, Format);
    vsnprintf(Buffer, 2048, Format.c_str(), vl);
    va_end(vl);
    logf << Buffer;
    logf.flush();
}

void Log::LogPrintf(std::string str, ...)
{
    char Buffer[2048];
    va_list vl;
    va_start(vl, str);
    vsnprintf(Buffer, 2048, str.c_str(), vl);
    va_end(vl);
    Printf(Buffer);
    Logf(Buffer);
}