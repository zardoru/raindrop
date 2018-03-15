VariantDir("build", "src", duplicate=0)

env = Environment(CXX='clang++', variant_dir='build')
env.Append(CPPPATH=[
	'src/ext',
	'src',
	'lib/include/LuaBridge',
	'lib/include/lua',
	'lib/include/stb',
	'lib/include/stdex',
	'lib/include'
])

IsDebug = ARGUMENTS.get('debug', 0)
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
import os 

if os.path.exists("src/pch.pch") and not IsDebug:
	env.Append(CXXFLAGS="-include-pch src/pch.pch")

if os.path.exists("src/dpch.pch") and not IsDebug:
	env.Append(CXXFLAGS="-include-pch src/dpch.pch")

env.Program("dc", source=[
	Glob('build/*.cpp'),
	Glob('build/ext/*.c'),
	Glob('build/ext/*.cpp')
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
