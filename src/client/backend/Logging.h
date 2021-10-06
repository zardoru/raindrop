#pragma once

namespace Log
{
    void DebugPrintf(std::string Format, ...);

    void Printf(std::string Format, ...);
    void Logf(std::string Format, ...);
    void LogPrintf(const std::string str, ...);
};