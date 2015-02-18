-- If true, then try and link libmpg123 and get mp3 support.

newoption {
	trigger = "with-mp3",
	description = "Enables looking for mpg123 for mp3 support."
}

newoption {
	trigger = "with-cygwin",
	description = "When enabled, set up environment for use with cygwin."
}

sdefines = {}

debuglibs = {
	"glew32d", "Ogg_d", "vorbis_d"
}

releaselibs = {
	"glew32", "Ogg", "vorbis"
}

libsearchdirs = {}

if os.is "windows" then
	-- These are only for windows.
	sdefines[#sdefines+1] = "WIN32"
	mp3lib = "libmpg123"
	sndfilelib = "libsndfile-1"
	portaudiolib = "portaudio_x86"
	soxrlib = "libsoxr"
	
	includelist = {
		"ext-libs/include"
	}
	
	plat = {"x32"}
	
	if _ACTION ~= "gmake" then
		includelist[#includelist+1] = "ext-libs/boost"
	else
		print("Using Cygwin or MinGW...")
		sdefines[#sdefines+1] = "HAS_STDINT"
	end
end

if os.is "linux" then
	sdefines[#sdefines+1] = "LINUX"
	sdefines[#sdefines+1] = "HAS_STDINT"
	mp3lib = "mpg123"
	sndfilelib = "sndfile"
	portaudiolib = "portaudio"
	soxrlib = "soxr"
	includelist = {}
	debuglibs = releaselibs -- No special suffix on linux..
	
	plat = {"native", "x32", "x64"}
end

if os.is "macosx" then
	-- This is mostly a copy of the linux stuff above.
	-- There aren't many differences between the two, but the subtle ones usually cause the most grief.
	sdefines[#sdefines+1] = "DARWIN"
	sdefines[#sdefines+1] = "HAS_STDINT"
	mp3lib = "mpg123"
	sndfilelib = "sndfile"
	portaudiolib = "portaudio"
	soxrlib = "soxr"

	includelist = {
		"/usr/local/include",
		"deps/ext-src/"
	}

	libsearchdirs = {
		"/usr/local/lib"
	}

	-- On OS X, the naming of some libraries is incorrect.
	-- Also, gotta tell clang the specific libraries to link against.
	releaselibs = {
		"glew",
		"glfw3",
		"portaudio",
		"rocketcontrols",
		"rocketcore",
		"rocketcorelua",
		"rocketcontrolslua",
		"sndfile",
		"mpg123",
		"lua",
		"iconv",
		"boost_thread-mt",
		"boost_system",
		"vorbisfile"
	}

	debuglibs = releaselibs
	plat = {"native", "x64"}
end

solution "raindrop-sln"
	configurations { "Debug", "Release" }
	platforms (plat)
	
	project "raindrop"
		language "C++"
		files { "src/*.cpp", "deps/ext-src/*.c", "deps/ext-src/SOIL/*.c", "deps/ext-src/*.cpp" }
		
		links ({ "glfw3", mp3lib, sndfilelib, soxrlib, portaudiolib })
		includedirs ("ext-src")
		includedirs (includelist)
        libdirs (libsearchdirs)
		defines (sdefines)

		configuration "with-mp3"
			links ("mpg123")
			defines { "MP3_ENABLED" }
		
		configuration "with-cygwin"
			defines { "__CYGWIN__" }
		
		configuration { "windows", "gmake" }
			defines { "MINGW", "__MINGW32__"}
			buildoptions { "-std=c++11" }
		
		configuration { "linux" }
			buildoptions { "-std=c++11" }

		configuration { "macosx" }
			buildoptions { "-std=c++11" }
			linkoptions {
				"-framework OpenGL",
				"-framework CoreFoundation",
				"-stdlib=libc++"
			}
		
		configuration "Debug"
			kind "consoleapp"
			libdirs { "ext-libs/Debug" }
			flags { "Symbols" }
			links (debuglibs)
		
		configuration "Release"
			kind "consoleapp"
			libdirs { "ext-libs/Release" }
			defines { "NDEBUG" }
			flags { "Optimize" }
			links (releaselibs)
	
