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

solution "raindrop-sln"
	configurations { "Debug", "Release" }
	platforms (plat)
	
	project "raindrop"
		language "C++"
		files { "src/*.cpp", "ext-src/*.c", "ext-src/SOIL/*.c", "ext-src/*.cpp" }
		
		links ({ "glfw3", mp3lib, sndfilelib, soxrlib, portaudiolib })
		includedirs ("ext-src")
		includedirs (includelist)
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
	
