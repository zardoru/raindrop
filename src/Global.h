#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <cstring>
#include <sstream>
#include <vector>
#include <algorithm>
#include <boost/cstdint.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>

using boost::function;
using boost::bind;
using boost::uint32_t;
using boost::int32_t;
using boost::int16_t;
using boost::uint16_t;
using boost::uint8_t;

typedef glm::vec2 Vec2;
typedef glm::mat4 Mat4;


typedef uint32_t uint32;
typedef int32_t int32;
typedef uint16_t int16;
typedef uint16_t uint16;
typedef uint8_t uint8;
#define GString std::string

extern float *PInfinity;

#define Infinity *PInfinity

#define M_PI	 3.14159265358979323846

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

double latof(GString s);

using std::max;
using std::min;

#include "Directory.h"

#ifdef WIN32
#pragma warning (disable: 4244)
#pragma warning (disable: 4996) // deprecation
#endif
