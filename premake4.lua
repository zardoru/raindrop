-- If true, then try and link libmpg123 and get mp3 support.

newoption {
	trigger = "with-mp3",
	description = "Enables looking for mpg123 for mp3 support."
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
		"ext-libs/include", "ext-libs/boost"
	}
end

if os.is "linux" then
	sdefines[#sdefines+1] = "LINUX"
	mp3lib = "mpg123"
	sndfilelib = "sndfile"
	portaudiolib = "portaudio"
	soxrlib = "soxr"
	includelist = {}
	debuglibs = releaselibs -- No special suffix on linux..
end

solution "raindrop-sln"
	configurations { "Debug", "Release" }
	platforms { "native", "x32", "x64" }
	
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
		
		configuration "Debug"
			kind "consoleapp"
			libdirs { "ext-libs/Debug" }
			links (debuglibs)
		
		configuration "Release"
			kind "consoleapp"
			libdirs { "ext-libs/Release" }
			defines { "NDEBUG" }
			links (releaselibs)
	
