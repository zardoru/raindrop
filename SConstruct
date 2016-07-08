
env = Environment(CPPPATH=['src'])

IsDebug = ARGUMENTS.get('release', 0)
DisableMP3 = ARGUMENTS.get('nomp3', 0)

env.Append(CPPDEFINES=['LINUX'], CXXFLAGS="-std=c++14")

if int(IsDebug) == 0:
	env.Append(CCFLAGS=["-g"])
else:
	env.Append(CCFLAGS=["-O2", "-DNDEBUG", "-fpermissive"])

if not int(DisableMP3):
	env.Append(CPPDEFINES=['MP3_ENABLED']) #another possible macro is NO_AUDIO but.. really now.
	env.Append(LIBS=['mpg123'])

import sys

env.Append(CPPPATH=['src/ext'])


env.Program("dc", source = [Glob('src/*.cpp'), Glob('src/*.c'), Glob('src/ext/*.c'), Glob('src/ext/*.cpp'), Glob('src/ext/SOIL/*.c')])

env.Append(LIBS=['rt', 'pthread', 'lua', 'dl', 'sndfile', 'GL', 'GLEW','boost_program_options','boost_filesystem','boost_thread', 'boost_system', 'ogg', 'vorbis', 'vorbisfile', 'png', 'jpeg', 'portaudio', 'soxr', 'glfw', 'sqlite3', 'X11', 'Xrandr', 'Xxf86vm', 'Xi', 'stdc++fs', 'RocketDebugger', 'RocketControls', 'RocketControlsLua', 'RocketCoreLua', 'RocketCore']);
