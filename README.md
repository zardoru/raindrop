raindrop
=====

raindrop is a small music game project aimed to be a multi-mode rhythm game for home desktops, nevertheless mainly focused on vsrg simulation (such as iidx, o2jam, stepmania, FTB, osu!mania, etc..). So far the game works but it's lacking on features.
A cytus-like mode is implemented and a beatmania-like mode is functional.

Nontheless, both modes still lack advanced features.

raindrop is built using OpenGL/PortAudio and most if not all are freely licensed libraries, while raindrop itself is licensed under the GPLv3.


Building & dependencies
=====
so while the code itself is fairly simple it has quite an amount of dependencies

* boost
* glew
* glfw 3.0
* portaudio v19
* libogg and libvorbis
* libsndfile
* glm

Building is basically setting up the include and library directories to all these
libraries (for msvc v90) after compiling them for this toolset
and once that's done you open the main project and fire up your f7 key.

you need scons to build it on linux and the same libraries.

Optionally, you may use libmpg123 to get MP3 support on either platform.

Notes
=====
Linux works, though you need the pa_memorybarrier.h file on both windows and linux.
I'm not including it since it's already part of the portaudio source code, which you should probably have if you're compiling this on your own.
