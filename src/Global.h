#include <exception>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <sstream>
#include <vector>

#include <boost/cstdint.hpp>

using boost::uint32_t;
using boost::int32_t;
using boost::int16_t;
using boost::uint16_t;

typedef glm::vec2 Vec2;
typedef glm::mat4 Mat4;


typedef uint32_t uint32;
typedef int32_t int32;
typedef uint16_t int16;
typedef uint16_t uint16;

#define String std::string

extern float *PInfinity;

#define Infinity *PInfinity

#include "Game_Consts.h"

#define M_PI	 3.14159265358979323846

namespace Utility
{
	void DebugBreak();
	bool IsNumeric(const char* s);
	String GetExtension(String Filename);
	String RelativeToPath(String Filename);
	String RemoveExtension(String Fn);
}

template <class T>
T abs (T x)
{
	return x > 0? x : -x;
}


#ifdef WIN32
#pragma warning (disable: 4244)
#pragma warning (disable: 4996) // deprecation
#endif
