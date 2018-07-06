VariantDir("build", "src", duplicate=0)
VariantDir("build-tests", "tests", duplicate=0)

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
BuildTests = ARGUMENTS.get('build-tests', 0)
BuildScripter = ARGUMENTS.get('build-scripter', 0)

env.Append(CPPDEFINES=['LINUX'], CXXFLAGS="-std=c++14")

if int(IsDebug):
	print "Compiling debug mode..."
	env.Append(CCFLAGS=["-g"])
else:
	print "Compiling release mode..."
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
	'vorbisfile',
	'z'
]);


common_src = [
	Glob('build/*.cpp'),
	Glob('build/ext/*.c'),
	Glob('build/ext/*.cpp')
]

env.StaticLibrary("rdshared", common_src)
env.Program("dc", LIBS=["rdshared"])

if BuildTests:
	tenv = env.Clone()
	tenv.Append(CPPDEFINES=['TESTS'])
	tenv.Program("dc-tests", LIBS=["rdshared"],
	source=[
		Glob('build-tests/tests/*.cpp')
	])

if BuildScripter:
	senv = env.Clone()
	senv.Append(CPPDEFINES=['SCRIPTS'])
	senv.Program("dc-lua", LIBS=["rdshared"],
	source=[
		Glob('build-tests/scripter/*.cpp')
	])

