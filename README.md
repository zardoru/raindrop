raindrop
=====

raindrop is a music game project aimed to be a multi-mode rhythm game for home desktops. It is a modern VSRG engine that supports warps, scroll speeds, and much more.

# Features

## Gameplay 
* Hidden/Fake notes
* OJN cover support
* Multiple timing systems
	* O2Jam (Near perfect simulation, with scoring)
	* Stepmania (excluding chord cohesion)
	* Raindrop Standard (with the EXP3 scoring system by Joe Zeng)
	* osu!mania (with the osu!mania scoring system)
	* EX-score scoring system
* Multiple Gauges
	* O2Jam Gauge
	* Death/Groove/Survival/ExHard/Easy
	* Stepmania gauge
* Hidden scrolling, user-adjustable
	* Flashlight, Hidden, Sudden
* Speed classes
	* CMod (As Constant)
	* Common (Or Mode, based on time rather than beats! The default!)
	* Max Speed
	* Min Speed
	* Green Number support
* Upscroll support
* Autoplay
* Random (By Lanes)
* Failure deactivation
* Scroll and/or Speed support for osu!, BMS and Stepmania charts

## UI
* Wheel sorting
* Lua Skinning with HTML-based librocket UI widgets
* TTF based SDF font support
* Result Histogram, Grades

## BGA
* Video playback via FFMPEG
* osu!mania storyboard support

## Rhythm Engine
* Negative-able BPM/stop support
* #RANDOM BMS
* Previewer commands to connect to bmsone or uBMSC/iBMSC
* MP3/Ogg/WAV support
* JPEG/PNG/BMP/TGA support, with an sRGB aware color space
* #PREVIEW extensions for BMS
* #MUSIC extension for BMS
* Automatic Chart Author extraction from #ARTIST tag

Formats supported
=====
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

## Video
* glew
* glfw
* libjpeg(turbo) 
* libpng
* ffmpeg

## Audio
* libogg 
* libvorbis/libvorbisfile
* libsndfile
* soxr
* portaudio
* portaudio v19
* libmpg123 (optional, but recommended)

## Other
* boost
	* gil
	* program options
* lua 5.2.1
* zlib
* librocket

## Included libraries
* glm (header only)
* LuaBridge (header only)
* stb_TrueType (header only)
* randint (header only)
* nlohmann's json (header only)
* utf8-cpp (header only)
* simpleini
* minizip
* portaudio's ringbuffer implementation
* sqlite3
* sha256 implementation by Stephan Brumme

Building on Windows
=====
Though you can compile them yourself, a full collection of pre-compiled libs can is available [here](https://www.dropbox.com/s/ck1jbxq5dfpz8h5/rdlib-18062018.7z?dl=0)
Make a `lib` directory at the same level as the `src` directory and extract the include and lib folders into it.
CMake should detect that you're on windows, and you've extracted these files. Otherwise, you'll have to provide them yourself.

Building on Linux
=====
To install the dependencies on Debian, run:
```
$ sudo apt install \
	clang \
	libavcodec-dev \
	libavformat-dev \
	libboost-all-dev \
	libglew-dev \
	libglfw3-dev \
	libglu1-mesa-dev \
	libjpeg62-turbo-dev \
	liblua5.2-dev \
	libmpg123-dev \
	libogg-dev \
	libpng-dev \
	libsndfile1-dev \
	libsoxr-dev \
	libsqlite3-dev \
	libswscale-dev \
	libvorbis-dev \
	libxi-dev \
	portaudio19-dev \
	zlib1g-dev
```

No package exists for libRocket, so you must compile it yourself.
It requires CMake and FreeType.
```
$ sudo apt install cmake libfreetype6-dev
$ git clone https://github.com/libRocket/libRocket.git
$ cd libRocket/Build
$ mkdir build
$ cd build
$ cmake -DBUILD_LUA_BINDINGS=ON ..
$ make
$ sudo make install
```

To compile raindrop, run:
```
$ scons
```

Building API documentation
=====
The API documentation of raindrop is built using LDoc. Once you install it, you can generate the API docs by running  ```ldoc .```
