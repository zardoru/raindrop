#ifndef GLOBAL_H_
#define GLOBAL_H_

#ifdef NIDEBUG
#undef _HAS_ITERATOR_DEBUGGING
#endif

#include <glm/glm.hpp>
#include <string>
#include <atomic>
#include <map>
#include <vector>
#include <algorithm>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <cctype>
#include <memory>

#if (defined _MSC_VER) && (_MSC_VER < 1800)
#error "You require Visual Studio 2013 to compile this application."
#endif

using std::atomic;
using std::function;
using std::bind;
using std::thread;
using std::mutex;
using std::unique_lock;
using std::condition_variable;
using namespace std::placeholders;

using std::shared_ptr;
using std::make_shared;
using std::dynamic_pointer_cast;
using std::static_pointer_cast;
using std::vector;
using std::map;
using std::sort;
using std::stable_sort;
using std::lower_bound;
using std::upper_bound;
using std::unique_ptr;

#include <stdint.h>

typedef glm::vec2 Vec2;
typedef glm::vec3 Vec3;
typedef glm::mat4 Mat4;


typedef uint32_t uint32;
typedef uint16_t uint16;
typedef uint8_t uint8;

typedef int32_t int32;
typedef int16_t int16;
typedef int8_t int8;

typedef std::string GString;

extern float *PInfinity;

#define Infinity *PInfinity

#ifndef DARWIN
#define M_PI	 3.14159265358979323846
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

typedef Fraction<long long> LFraction;
typedef Fraction<int> IFraction;

int b36toi(const char *txt);
int b16toi(const char *txt);

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
	GString GetSha256ForFile(GString Filename);
	GString IntToStr(int num);
	GString CharToStr(char c);
	void RemoveFilenameIllegalCharacters(GString &S, bool removeSlash, bool noAbsolute = true);

	GString Format(GString str, ...);


	vector<GString> TokenSplit(const GString &str, const GString &token = ",", bool compress = false);

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

int LCM(const vector<int> &Set);
double latof(GString s);

using std::max;
using std::min;

#include "Directory.h"

#ifdef WIN32
#pragma warning (disable: 4244) // possible loss of data
#pragma warning (disable: 4996) // deprecation
#pragma warning (disable: 4800) // cast from bool to int
#endif

#endif