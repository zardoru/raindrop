#ifndef GLOBAL_H_
#define GLOBAL_H_

#ifdef NIDEBUG
#undef _HAS_ITERATOR_DEBUGGING
#endif

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <cstring>
#include <sstream>
#include <vector>
#include <algorithm>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <memory>

#if (defined _MSC_VER) && (_MSC_VER < 1800)
#error "You require Visual Studio 2013 to compile this application."
#endif

using boost::function;
using boost::bind;

using std::shared_ptr;
using std::make_shared;
using std::dynamic_pointer_cast;
using std::static_pointer_cast;
using std::vector;

// Use stdint if available.
#if !(defined HAS_STDINT) && ( !(defined _MSC_VER) || (_MSC_VER < 1800) )
#include <boost/cstdint.hpp>
using boost::uint32_t;
using boost::uint16_t;
using boost::uint8_t;
using boost::int32_t;
using boost::int16_t;
using boost::int8_t;
#else
#include <stdint.h>
#endif

typedef glm::vec2 Vec2;
typedef glm::vec3 Vec3;
typedef glm::mat4 Mat4;


typedef uint32_t uint32;
typedef uint16_t uint16;
typedef uint8_t uint8;

typedef int32_t int32;
typedef int16_t int16;
typedef int8_t int8;

#define GString std::string

extern float *PInfinity;

#define Infinity *PInfinity

#define M_PI	 3.14159265358979323846

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

typedef TAABB<float> AABB;
typedef TAABB<double> AABBd;

template
<class T>
struct TColorRGB {
	union {
		struct { T R, G, B, A; };
		struct { T Red, Green, Blue, Alpha; };
	};
};

typedef TColorRGB<float> ColorRGB;
typedef TColorRGB<double> ColorRGBd;

namespace Color {
	extern const ColorRGB White;
	extern const ColorRGB Black;
	extern const ColorRGB Red;
	extern const ColorRGB Green;
	extern const ColorRGB Blue;
}

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
	void CheckDir (GString Dirname);
	int GetLMT(GString Path);
	void RemoveFilenameIllegalCharacters(GString &S, bool removeSlash = false);
	GString getSha256ForFile(GString Filename);
	GString IntToStr(int num);
	GString CharToStr(char c);
}


enum KeyEventType
{
	KE_None,
	KE_Press,
	KE_Release
};

template <class T>
T abs (T x)
{
	return x > 0? x : -x;
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
	while(output > target + interval) output -= interval * 2;
	while(output < target - interval) output += interval * 2;
	return output;
}

double latof(GString s);

using std::max;
using std::min;

#include "Directory.h"

#ifdef WIN32
#pragma warning (disable: 4244)
#pragma warning (disable: 4996) // deprecation
#endif

#endif