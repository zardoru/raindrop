#pragma once

#ifdef NIDEBUG
#undef _HAS_ITERATOR_DEBUGGING
#endif

#if (defined _MSC_VER) && (_MSC_VER < 1800)
#error "You require Visual Studio 2013 to compile this application."
#endif

#ifdef WIN32
#pragma warning (disable: 4244) // possible loss of data
#pragma warning (disable: 4996) // deprecation
#pragma warning (disable: 4800) // cast from bool to int

#define ENABLE_SNDFILE_WINDOWS_PROTOTYPES 1
#include <Windows.h>
#include <io.h>
#include <fcntl.h>
#include <pa_win_wasapi.h>
#include <pa_win_ds.h>

#ifndef MINGW
#include <direct.h>
#include <mpg123.h>
#else
#include <mpg123mingw.h>
#endif

#else
#include <iconv.h>
#endif

#ifdef LINUX
#include <pa_linux_alsa.h>
#endif

#if _MSC_VER >= 1900
#include <filesystem>
namespace std {
    namespace filesystem = experimental::filesystem;
}
#else
#include <boost\filesystem.hpp>
namespace std {
    namespace filesystem = boost::filesystem;
}
#endif

#include <algorithm>
#include <atomic>
#include <chrono>
#include <codecvt>
#include <condition_variable>
#include <exception>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <numeric>
#include <queue>
#include <random>
#include <regex>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

#include <cctype>
#include <clocale>
#include <cmath>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>

// #include <boost\gil\extension\io\bmp_all.hpp>
// #include <boost\gil\extension\io\png_all.hpp>
// #include <boost\gil\extension\io\jpeg_all.hpp>

#include <boost\program_options.hpp>
#include <boost\interprocess\ipc\message_queue.hpp>

#include <randint.h>

#include <Rocket\Core.h>
#include <Rocket\Controls.h>
#include <Rocket\Debugger.h>
#include <Rocket\Core\Lua\Interpreter.h>
#include <Rocket\Controls\Lua\Controls.h>

#include <FreeImage.h>

#include <LuaBridge.h>
extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

#include <json\json.h>

#include <GL\glew.h>
#include <glm\glm.hpp>
#include <glm\gtc\type_ptr.hpp>
#include <GLFW\glfw3.h>

#include <sqlite3.h>

#include <sys\stat.h>

#include <ogg\ogg.h>
#include <vorbis\vorbisfile.h>

#include <portaudio.h>
#include <pa_ringbuffer.h>

#include <sndfile.h>

#include <soxr.h>

using Vec2 = glm::vec2;
using Vec3 = glm::vec3;
using Mat4 = glm::mat4;

using GString = std::string;

extern float *PInfinity;

#define Infinity *PInfinity

#ifndef DARWIN
#define M_PI 3.14159265358979323846
#endif

template
<class T>
struct TAABB {
    union {
        struct { T X, Y; } P1;
        struct { T X1, Y1; }; // Topleft point
    };

    union {
        struct { T X, Y; } P2;
        struct { T X2, Y2; }; // Bottomright point
    };
};

using AABB = TAABB<float>;
using AABBd = TAABB<double>;

template
<class T>
struct TColorRGB {
    union {
        struct { T R, G, B, A; };
        struct { T Red, Green, Blue, Alpha; };
    };
};

using ColorRGB = TColorRGB<float>;
using ColorRGBd = TColorRGB<double>;

namespace Color {
    extern const ColorRGB White;
    extern const ColorRGB Black;
    extern const ColorRGB Red;
    extern const ColorRGB Green;
    extern const ColorRGB Blue;
}

template <class T>
struct Fraction {
    T Num;
    T Den;

    Fraction()
    {
        Num = Den = 1;
    }

    template <class K>
    Fraction(K num, K den) {
        Num = num;
        Den = den;
    }

    void fromDouble(double in)
    {
        double d = 0;
        Num = 0;
        Den = 1;
        while (d != in)
        {
            if (d < in)	++Num;
            else if (d > in) ++Den;
            d = static_cast<double>(Num) / Den;
        }
    }
};

using LFraction = Fraction<long long>;
using IFraction = Fraction<int>;


namespace Utility
{
    void DebugBreak();
    bool IsNumeric(const char* s);
    GString RelativeToPath(GString Filename);
    GString RemoveExtension(GString Fn);
    bool FileExists(GString Filename);
    std::wstring Widen(GString Line);
    GString Narrow(std::wstring Line);
    GString SJIStoU8(GString Line);
    void CheckDir(GString Dirname);
    int GetLMT(GString Path);
    GString GetSha256ForFile(GString Filename);
    GString IntToStr(int num);
    GString CharToStr(char c);
    void RemoveFilenameIllegalCharacters(GString &S, bool removeSlash, bool noAbsolute = true);

    GString Format(GString str, ...);


    std::vector<GString> TokenSplit(const GString &str, const GString &token = ",", bool compress = false);

    GString Trim(GString& str);
    GString ReplaceAll(GString& str, const GString& seq, const GString what);
    GString ToLower(GString& str); // Caveat: only for ascii purposes.

    template <class T>
    GString Join(const T& iterable, const GString& seq)
    {
        GString ret;
        for (auto s = iterable.begin(); s != iterable.end(); ++s)
        {
            auto next = s; ++next;
            if (next != iterable.end())
                ret += *s + seq;
            else
                ret += *s;
        }

        return ret;
    }
}

enum KeyEventType
{
    KE_NONE,
    KE_PRESS,
    KE_RELEASE
};

template <class T>
T abs(T x)
{
    return x > 0 ? x : -x;
}

inline bool IntervalsIntersect(const double a, const double b, const double c, const double d)
{
    return a <= d && c <= b;
}

template <class T>
inline T LerpRatio(const T &Start, const T& End, double Progress, double Total)
{
    return Start + (End - Start) * Progress / Total;
}

template <class T>
inline T Lerp(const T &Start, const T& End, double k)
{
    return Start + (End - Start) * k;
}

template <class T>
inline T Clamp(const T &Value, const T &Min, const T &Max)
{
    if (Value < Min) return Min;
    else if (Value > Max) return Max;
    else return Value;
}

template <class T>
inline T clamp_to_interval(const T& value, const T& target, const T& interval)
{
    T output = value;
    while (output > target + interval) output -= interval * 2;
    while (output < target - interval) output += interval * 2;
    return output;
}

int LCM(const std::vector<int> &Set);
double latof(GString s);

#include "directory.h"