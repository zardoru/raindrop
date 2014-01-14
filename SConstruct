env = Environment(CPPPATH=['src'])

env.Append(CCFLAGS=["-g"])
env.Append(CPPDEFINES=['ILUT_USE_OPENGL', 'DEBUG', 'DSFMT_MEXP=216091'])
env.Append(LIBS= ['GL', 'GLU', 'GLEW', 'glfw3', 'X11', 'Xrandr', 'Xxf86vm', 'Xi', 'sfml-system', 'sfml-window', 'sfml-graphics', 'boost_system', 'boost_timer', 'boost_thread', 'ogg', 'vorbis', 'vorbisfile', 'portaudio', 'pthread'])

env.Program("dotcur.exe", source = [Glob('src/*.cpp'), Glob('src/*.c'), Glob('soil/*.c')])
