#include <fstream>
#include "Global.h"
#include "TruetypeFont.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

TruetypeFont::TruetypeFont(Directory Filename)
{
	std::ifstream ifs (Filename.c_path(), std::ios::binary);
	
	if (!ifs.is_open())
		assert(0);

	// tell size
	ifs.seekg(0, ifs.end);
	offs = ifs.tellg();
	ifs.seekg(0, ifs.beg);

	data = new unsigned char[offs];
	info = new stbtt_fontinfo;

	// read data
	ifs.read((char*)data, offs);

	if (!stbtt_InitFont(info, data, stbtt_GetFontOffsetForIndex(data, 0)))
		assert (0);
	SetupTexture();
}

