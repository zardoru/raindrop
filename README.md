raindrop
=====

raindrop is a music game project aimed to be a multi-mode rhythm game for home desktops, nevertheless mainly focused on vsrg simulation (such as iidx, o2jam, stepmania, FTB, osu!mania, etc.). 
A cytus-like mode is implemented and a beatmania-like mode is functional.

raindrop is built using OpenGL/PortAudio and most if not all are freely licensed libraries, while raindrop itself is licensed under the GPLv3.


Building & dependencies
=====
So while the code itself is fairly simple it has quite an amount of dependencies

* boost
* glew
* glfw 3.0
* portaudio v19
* libogg and libvorbis
* libsndfile
* glm
* librocket

Building is basically setting up the include and library directories to all these libraries (for Visual Studio 2013) after compiling them for this toolset and once that's done you open the main project and fire up your f7 key.

you need scons to build it on linux and the same libraries.

Optionally, you may use libmpg123 to get MP3 support on either platform.

Notes
=====
Extract deps.zip into the project's root folder to get most of the dependencies covered on windows. You still need ext-src as of now on linux.
