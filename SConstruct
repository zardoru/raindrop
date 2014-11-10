env = Environment(CPPPATH=['src'])

IsDebug = ARGUMENTS.get('release', 0)
DisableMP3 = ARGUMENTS.get('nomp3', 0)

env.Append(CPPDEFINES=['LINUX'], CXXFLAGS="-std=c++0x")

if int(IsDebug) == 0:
	env.Append(CCFLAGS=["-g"])
else:
	env.Append(CCFLAGS=["-O2", "-DNDEBUG"])

if not int(DisableMP3):
	env.Append(CPPDEFINES=['MP3_ENABLED']) #another possible macro is NO_AUDIO but.. really now.
	env.Append(LIBS=['mpg123'])

import sys

env.Append(CPPPATH=['ext-src'])

env.Program("dc", source = [Glob('src/*.cpp'), Glob('src/*.c'), Glob('ext-src/*.c'), Glob('ext-src/*.cpp'), Glob('ext-src/SOIL/*.c')])

env.Append(LIBS=['dl', 'pthread', 'sndfile', 'GL', 'GLEW', 'glfw3', 'boost_system', 'boost_thread', 'ogg', 'vorbis', 'vorbisfile', 'portaudio', 'soxr', 'X11', 'Xrandr', 'Xxf86vm', 'Xi']);
