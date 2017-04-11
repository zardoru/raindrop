raindrop
=====

raindrop is a music game project aimed to be a multi-mode rhythm game for home desktops, nevertheless mainly focused on vsrg simulation (such as iidx, o2jam, stepmania, FTB, osu!mania, etc.). 
A cytus-like mode is implemented and a flexible VSRG engine is, too. It supports several chart formats and is able to convert between them, including:

* bms/bme/bml/pms (+ raindrop-specific extensions)
* bmson
* o2jam ojn/ojm
* osu!mania
* stepmania 3.9/5.0 (.sm/.ssc, with warps support, though no keysounds or delays.)

For several of these, the major mechanics varying between them are coded into raindrop, and are activated depending on the chart format.
raindrop is built using OpenGL/PortAudio and most if not all are freely licensed libraries, while raindrop itself is licensed under the GPLv3.

[![Build status](https://ci.appveyor.com/api/projects/status/muhxwis6usx75hhn?svg=true)](https://ci.appveyor.com/project/zardoru/raindrop)


Dependencies
=====
The dependencies of the project right now are:

* boost and boost::gil (develop)
* glew
* glfw 3.0
* portaudio v19
* libogg and libvorbis
* libsndfile
* soxr
* portaudio
* librocket
* sqlite3
* libjpeg(turbo) and libpng
* lua 5.2.1
* zlib
* libmpg123 (optional, but recommended)
* glm (header only)
* LuaBridge (header only)
* stb_TrueType (header only)
* randint (header only)

With the exception of boost, the required includes for these dependencies are within the 'lib\include' folder.
Header only libraries require nothing more to be used.


Building on Windows
=====
Though you can compile them yourself, a full collection of pre-compiled libs can is available [here](https://www.dropbox.com/s/alkxtb2tozllaap/rdlib-09042017.7z?dl=0)
If you're using Visual Studio, simply extracting these to the 'lib' directory will allow automatic linking.
The solution will attempt to grab boost from NuGet first time, though it can simple be added manually if desired.


Building on Linux
=====
The Linux scons file as of right now is outdated. 
Not only that, you've got to hunt for the dependencies yourself, even if it wasn't.
As of right now, it still is a decent base to get it to build.
