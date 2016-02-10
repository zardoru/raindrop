#include "pch.h"

#include "TruetypeFont.h"
#include "GameWindow.h"

#include "Logging.h"

class TTFMan {
public:
	struct FontData {
		std::shared_ptr<std::vector<unsigned char>> data;
		std::shared_ptr<stbtt_fontinfo> info;
	};

private:
	static std::map<std::filesystem::path, FontData> font_data;

public:
	static void Load(std::filesystem::path Filename, 
		std::shared_ptr<std::vector<unsigned char>>& data, std::shared_ptr<stbtt_fontinfo>& info, bool &IsValid) {

		// accelerate loading if font is on registers
		if (font_data.find(Filename) != font_data.end())
		{
			auto &fnt = font_data[Filename];
			info = fnt.info;
			data = fnt.data;
			IsValid = true;
			return;
		}

		std::ifstream ifs(Filename.c_str(), std::ios::binary);

		info = nullptr;
		data = nullptr;

		if (!ifs.is_open())
		{
			IsValid = false;
			return;
		}

		// tell size
		ifs.seekg(0, ifs.end);
		auto offs = ifs.tellg();
		ifs.seekg(0, ifs.beg);

		data = std::make_shared<std::vector<unsigned char>>(offs);
		info = std::make_shared<stbtt_fontinfo>();

		// read data
		ifs.read((char*)data.get()->data(), offs);

		IsValid = (stbtt_InitFont(info.get(), 
			data.get()->data(), 
			stbtt_GetFontOffsetForIndex(data.get()->data(), 
				0)) != 0);

		if (IsValid) {
			font_data[Filename] = {
				data,
				info
			};
		}
	}
};

std::map < std::filesystem::path, TTFMan::FontData > TTFMan::font_data;

TruetypeFont::TruetypeFont(std::filesystem::path Filename, float Scale)
{
	TTFMan::Load(Filename, this->data, this->info, IsValid);

	scale = Scale;
	windowscale = 0;

    if (IsValid)
    {
        UpdateWindowScale();

        WindowFrame.AddTTF(this);
    }
    else
        Log::Printf("Failure loading TTF file %s.\n", Utility::Narrow(Filename.wstring()).c_str());
}

TruetypeFont::~TruetypeFont()
{
    WindowFrame.RemoveTTF(this);
    ReleaseTextures();
}

void TruetypeFont::UpdateWindowScale()
{
    if (windowscale == WindowFrame.GetWindowVScale())
        return;

    float oldscale = windowscale;
    windowscale = WindowFrame.GetWindowVScale();

    float oldrealscale = realscale;
    realscale = stbtt_ScaleForPixelHeight(info.get(), scale * windowscale);
    virtualscale = stbtt_ScaleForPixelHeight(info.get(), scale);
#ifdef VERBOSE_DEBUG
    wprintf(L"change scale %f -> %f, realscale %f -> %f\n", oldscale, windowscale, oldrealscale, realscale);
#endif
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

void TruetypeFont::CheckCodepoint(int cp)
{
    if (Texes.find(cp) != Texes.end())
    {
        if (Texes[cp].scl != windowscale)
        {
#ifdef VERBOSE_DEBUG
            wprintf(L"releasing %d\n", cp);
#endif
            ReleaseCodepoint(cp); // force regeneration if scale changed
        }
    }
}

TruetypeFont::codepdata &TruetypeFont::GetTexFromCodepoint(int cp)
{
    if (Texes.find(cp) == Texes.end())
    {
        codepdata newcp;
        int w, h, xofs, yofs;
#ifdef VERBOSE_DEBUG
        wprintf(L"generating %d\n", cp);
#endif
        if (IsValid)
        {
            newcp.tex = stbtt_GetCodepointBitmap(info.get(), 0, realscale, cp, &w, &h, &xofs, &yofs);
            newcp.gltx = 0;
            newcp.scl = WindowFrame.GetWindowVScale();
            newcp.tw = w;
            newcp.th = h;

            // get size etc.. for how it'd be if the screen weren't resized
            void * tx = stbtt_GetCodepointBitmap(info.get(), 0, virtualscale, cp, &w, &h, &xofs, &yofs);
            newcp.xofs = xofs;
            newcp.yofs = yofs;
            newcp.w = w;
            newcp.h = h;

            free(tx);
        }
        else
            memset(&newcp, 0, sizeof(codepdata));

        Texes[cp] = newcp;
        return Texes[cp];
    }
    else
    {
        return Texes[cp];
    }
}

float TruetypeFont::GetHorizontalLength(const char *In)
{
    const char* Text = In;
    size_t len = strlen(In);
    float Out = 0;

    if (!IsValid) return 0;

    try
    {
        utf8::iterator<const char*> it(Text, Text, Text + len);
        utf8::iterator<const char*> itend(Text + len, Text, Text + len);
        for (; it != itend; ++it)
        {
            CheckCodepoint(*it); // Force a regeneration of this if necessary
            codepdata &cp = GetTexFromCodepoint(*it);

            auto it_nx = it;
            ++it_nx;
            if (it_nx != itend)
            {
                float aW = stbtt_GetCodepointKernAdvance(info.get(), *it, *it_nx);
                int bW;
                stbtt_GetCodepointHMetrics(info.get(), *it, &bW, NULL);
                Out += aW * virtualscale + bW * virtualscale;
            }
            else
                Out += cp.w;
        }
    }
    catch (...)
    {
    }

    return Out;
}