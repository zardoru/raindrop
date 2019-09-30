#!/bin/bash
clang++  -std=c++14 -x c++-header -O2 -DLINUX -DMP3_ENABLED -Isrc/ext -Isrc -Ilib/include/LuaBridge -Ilib/include/lua -Ilib/include/stb -Ilib/include/stdex -Ilib/include src/pch.h -o src/pch.pch
