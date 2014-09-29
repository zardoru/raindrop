#include <fstream>
#include "Global.h"
#include "TruetypeFont.h"
#include "GameWindow.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

TruetypeFont::TruetypeFont(Directory Filename, float Scale)
{
	std::ifstream ifs (Filename.c_path(), std::ios::binary);
	
	if (!ifs.is_open())
	{
		IsValid = false;
		return;
	}

	scale = Scale;

	// tell size
	ifs.seekg(0, ifs.end);
	offs = ifs.tellg();
	ifs.seekg(0, ifs.beg);

	data = new unsigned char[offs];
	info = new stbtt_fontinfo;

	// read data
	ifs.read((char*)data, offs);

	IsValid = (stbtt_InitFont(info, data, stbtt_GetFontOffsetForIndex(data, 0)) != 0);

	if (IsValid)
	{
		SetupTexture();
		realscale = stbtt_ScaleForPixelHeight(info, scale);
		WindowFrame.AddTTF(this);
	}
}

TruetypeFont::~TruetypeFont()
{
	WindowFrame.RemoveTTF(this);
	delete data;
	delete info;
	ReleaseTextures();
}

void TruetypeFont::Invalidate()
{
	for (std::map <int, codepdata>::iterator i = Texes.begin();
		i != Texes.end();
		i++)
	{
		i->second.gltx = 0;
	}
}

TruetypeFont::codepdata &TruetypeFont::GetTexFromCodepoint(int cp)
{
	if (Texes.find(cp) == Texes.end())
	{
		codepdata newcp;
		int w, h, xofs, yofs;
		newcp.tex = stbtt_GetCodepointBitmap (info, 0, realscale, cp, &w, &h, &xofs, &yofs);
		newcp.xofs = xofs;
		newcp.yofs = yofs;
		newcp.w = w;
		newcp.h = h;
		newcp.gltx = 0;
		Texes[cp] = newcp;

		return Texes[cp];
	}else
	{
		return Texes[cp];
	}
}
