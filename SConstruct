env = Environment(CPPPATH=['src'])

env.Append(CCFLAGS=["-g"])

# take the MP3_ENABLED out if you feel you hate mp3s
env.Append(CPPDEFINES=['DEBUG', 'MP3_ENABLED']) #another possible macro is NO_AUDIO but.. really now.

# and here remove mpg123 from the list if you don't even have libmpg123
env.Append(LIBS= ['GL', 'GLU', 'GLEW', 'glfw3', 'X11', 'Xrandr', 'mpg123', 'Xxf86vm', 'Xi', 'boost_system', 'boost_thread', 'ogg', 'vorbis', 'vorbisfile', 'portaudio', 'pthread'])
env.Append(CPPPATH='.')

env.Program("dotcur.exe", source = [Glob('src/*.cpp'), Glob('src/*.c'), Glob('SOIL/*.c')])
