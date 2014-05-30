#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <cstring>
#include <sstream>
#include <vector>

#include <boost/cstdint.hpp>

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
#define String std::string

extern float *PInfinity;

#define Infinity *PInfinity

#define M_PI	 3.14159265358979323846

namespace Utility
{
	void DebugBreak();
	bool IsNumeric(const char* s);
	String GetExtension(String Filename);
	String RelativeToPath(String Filename);
	String RemoveExtension(String Fn);
	bool FileExists(String Filename);
	std::wstring Widen(String Line);
	String Narrow(std::wstring Line);
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

using std::max;
using std::min;

#ifdef WIN32
#pragma warning (disable: 4244)
#pragma warning (disable: 4996) // deprecation
#endif
