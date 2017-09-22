env = Environment(CXX='clang++')
env.Append(CPPPATH=[
	'src/ext',
	'src',
	'lib/include/LuaBridge',
	'lib/include/lua',
	'lib/include/stb',
	'lib/include/stdex',
	'lib/include'
])

IsDebug = ARGUMENTS.get('release', 0)
DisableMP3 = ARGUMENTS.get('nomp3', 0)

env.Append(CPPDEFINES=['LINUX'], CXXFLAGS="-std=c++14")

if int(IsDebug):
	env.Append(CCFLAGS=["-g"])
else:
	env.Append(CCFLAGS=["-O2", "-DNDEBUG", "-fpermissive"])

if not int(DisableMP3):
	env.Append(CPPDEFINES=['MP3_ENABLED']) #another possible macro is NO_AUDIO but.. really now.
	env.Append(LIBS=['mpg123'])

import sys

env.Program("dc", source=[
	Glob('src/*.cpp'),
	Glob('src/ext/*.c'),
	Glob('src/ext/*.cpp')
])

env.Append(LIBS=[
	'avcodec',
	'avformat',
	'avutil',
	'boost_filesystem',
	'boost_program_options',
	'boost_system',
	'GL',
	'GLEW',
	'glfw',
	'jpeg',
	'lua5.2',
	'ogg',
	'png',
	'portaudio',
	'pthread',
	'RocketControls',
	'RocketControlsLua',
	'RocketCore',
	'RocketCoreLua',
	'rt',
	'sndfile',
	'soxr',
	'sqlite3',
	'stdc++fs',
	'swscale',
	'vorbis',
	'vorbisfile'
]);
