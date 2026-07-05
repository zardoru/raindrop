#pragma once

namespace Log
{
    void DebugPrintf(std::string Format, ...);

    void Printf(const std::string& format, ...);
    void Logf(const std::string &format, ...);
    void LogPrintf(const std::string &str, ...);
};