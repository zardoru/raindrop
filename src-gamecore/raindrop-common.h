#ifndef PCH_H_
#define PCH_H_

#if (defined _MSC_VER) && (_MSC_VER < 1800)
#error "You require Visual Studio 2013 or higher to compile this application."
#endif

#ifndef WIN32
#include <iconv.h>
#endif



// STL
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
#include <streambuf>
#include <string>
#include <future>
#include <thread>
#include <unordered_set>
#include <vector>

#include <randint> // C++17 example implementation

// C stdlib
#include <cctype>
#include <clocale>
#include <cmath>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>


// others
#include <sqlite3.h>
#include <LuaBridge.h>
#include <sys/stat.h>

// static libraries
#include "ext/json.hpp"
#include "ext/utf8.h"



using Json = nlohmann::json;

template
<class T>
struct TAABB
{
    union
    {
        struct { T X, Y; } P1;
        struct { T X1, Y1; }; // Topleft point
    };

    union
    {
        struct { T X, Y; } P2;
        struct { T X2, Y2; }; // Bottomright point
    };

	TAABB(T x1, T y1, T x2, T y2) {
		X1 = x1;
		X2 = x2;
		Y1 = y1;
		Y2 = y2;
	}

	TAABB() {
		X1 = X2 = 0;
		Y1 = Y2 = 0;
	}

	inline bool IsInBox(T x, T y) {
		return x >= X1 && x <= X2
			&& y >= Y1 && y <= Y2;
	}

	inline bool Intersects(const TAABB &other) {
		return IsInBox(other.X1, other.Y1) ||
			IsInBox(other.X2, other.Y2) ||
			IsInBox(other.X2, other.Y1) ||
			IsInBox(other.X1, other.Y2);
	}

	inline void SetWidth(T w) {
		X2 = X1 + w;
	}

	inline void SetHeight(T h) {
		Y2 = Y1 + h;
	}

	T width() const { 
		return X2 - X1;
	}

	T height() const {
		return Y2 - Y1;
	}
};

using AABB = TAABB<float>;
using AABBd = TAABB<double>;

template
<class T>
struct TColorRGB
{
    union
    {
        struct { T R, G, B, A; };
        struct { T Red, Green, Blue, Alpha; };
    };
};

using ColorRGB = TColorRGB<float>;
using ColorRGBd = TColorRGB<double>;

namespace Color
{
    extern const ColorRGB White;
    extern const ColorRGB Black;
    extern const ColorRGB Red;
    extern const ColorRGB Green;
    extern const ColorRGB Blue;
}












#endif // #define PCH_H_