#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstdarg>

#include <boost/nowide/iostream.hpp>
#include "Logging.h"

#include <chrono>
#include <format>

using std::wcout;
static std::fstream logfile("log.txt", std::ios::out);

#ifndef NDEBUG
void Log::DebugPrintf(std::string Format, ...) {
    LogPrintf(Format);
}
#else
void Log::DebugPrintf(std::string Format, ...) {
    // stub
}
#endif

#define VARSNPF(fmt, blk) { \
    char buffer[2048];\
    const char* ptr = buffer;\
    va_list vl;\
    va_start(vl, format);\
    auto nsize = vsnprintf(buffer, 2048, fmt.c_str(), vl);\
    va_end(vl);\
    if (nsize > sizeof buffer) {\
        ptr = new char[nsize + 1];\
        va_list v2;\
        va_start(v2, format);\
        vsnprintf(buffer, nsize + 1, fmt.c_str(), v2);\
        va_end(v2);\
        { blk; } \
        delete[] ptr; \
    } else { \
        { blk; } \
    } \
}

void Log::Printf(const std::string &format, ...) {
    VARSNPF(format, {
        boost::nowide::cout << std::format("[{:%F %T}] {}", std::chrono::system_clock::now(), ptr);
    });
}

void Log::Logf(const std::string &format, ...) {
    VARSNPF(format, {
        auto now = std::chrono::system_clock::now();
        auto t = std::format("[{:%F %T}] {}", now, ptr);
        logfile << t;
        logfile.flush();
    });
}

void Log::LogPrintf(const std::string &format, ...) {
    VARSNPF(format, {
        auto now = std::chrono::system_clock::now();
        auto s = std::format("[{:%F %T}] {}", now, ptr);
        boost::nowide::cout << s;
        logfile << s; logfile.flush();
    });
}
