#ifdef WIN32
#pragma warning (disable: 4244) // possible loss of data
#pragma warning (disable: 4996) // deprecation
#pragma warning (disable: 4800) // cast from bool to int

#define _USE_MATH_DEFINES

#define ENABLE_SNDFILE_WINDOWS_PROTOTYPES 1
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <io.h>
#include <fcntl.h>
#include <pa_win_wasapi.h>
#include <pa_win_ds.h>
#endif

#include <stb/stb_truetype.h>

// OpenGL
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// audio
#include <portaudio/portaudio.h>

enum KeyEventType
{
    KE_NONE,
    KE_PRESS,
    KE_RELEASE
};