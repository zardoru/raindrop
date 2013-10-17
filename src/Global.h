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


typedef uint32_t uint32;
typedef int32_t int32;
typedef uint16_t int16;
typedef uint16_t uint16;

#define String std::string

extern float *PInfinity;

#define Infinity *PInfinity

#include "Game_Consts.h"

namespace Utility
{
	void DebugBreak();
	bool IsNumeric(const char* s);
	String GetExtension(String Filename);
	String RelativeToPath(String Filename);
}

#ifdef WIN32
#pragma warning (disable: 4244)
#pragma warning (disable: 4996) // deprecation
#endif

//#ifndef OLD_GL
// #define DISABLE_CEGUI
//#endif