#include "pch.h"

#include "TruetypeFont.h"

#include "GameState.h"
#include "GameWindow.h"

#include "TTFCache.h"
#include "Logging.h"

#include "SDF.h"

const std::filesystem::path CACHE_PATH = "GameData/fontcache/";

class TTFMan {
public:
	struct FontData {
		std::shared_ptr<std::vector<unsigned char>> data;
		std::shared_ptr<stbtt_fontinfo> info;
		std::shared_ptr<std::map<int, TruetypeFont::codepdata> > texels;
	};

private:
	static std::map<std::filesystem::path, FontData> font_data;

public:
	static void Load(std::filesystem::path Filename, 
		std::shared_ptr<std::vector<unsigned char>>& data, 
		std::shared_ptr<stbtt_fontinfo>& info, 
		std::shared_ptr<std::map<int, TruetypeFont::codepdata> > &Texels,
		bool &IsValid) {

		// accelerate loading if font is on registers
		if (font_data.find(Filename) != font_data.end())
		{
			auto &fnt = font_data[Filename];
			info = fnt.info;
			data = fnt.data;
			Texels = fnt.texels;
			IsValid = true;
			return;
		}

		std::ifstream ifs(Filename.string(), std::ios::binary);

		info = nullptr;
		data = nullptr;
		Texels = nullptr;

		if (!ifs.is_open())
		{
			Log::LogPrintf("Couldn't open font %s\n", Filename.string().c_str());
			IsValid = false;
			return;
		}

		// tell size
		ifs.seekg(0, ifs.end);
		auto offs = ifs.tellg();
		ifs.seekg(0, ifs.beg);

		data = std::make_shared<std::vector<unsigned char>>(offs);
		info = std::make_shared<stbtt_fontinfo>();
		Texels = std::make_shared<std::map<int, TruetypeFont::codepdata>>();
		// read data
		ifs.read((char*)data.get()->data(), offs);

		IsValid = (stbtt_InitFont(info.get(), 
			data.get()->data(), 
			stbtt_GetFontOffsetForIndex(data.get()->data(), 
				0)) != 0);

		if (IsValid) {
			font_data[Filename] = {
				data,
				info,
				Texels
			};
		}
		else {
			Log::LogPrintf("Couldn't load font after reading it\n");
		}
	}
};

std::map < std::filesystem::path, TTFMan::FontData > TTFMan::font_data;

TruetypeFont::TruetypeFont(std::filesystem::path Filename)
{
	TTFMan::Load(Filename, this->data, this->info, this->Texes, IsValid);
	
    if (IsValid)
    {
		realscale = stbtt_ScaleForPixelHeight(info.get(), SDF_SIZE);
        WindowFrame.AddTTF(this);
    }
    else
        Log::Printf("Failure loading TTF file %s.\n", Utility::ToU8(Filename.wstring()).c_str());
}

TruetypeFont::~TruetypeFont()
{
    WindowFrame.RemoveTTF(this);
    // ReleaseTextures();
}

void TruetypeFont::Invalidate()
{
    for (std::map <int, codepdata>::iterator i = Texes->begin();
    i != Texes->end();
        i++)
    {
        i->second.gltx = 0;
    }
}


TruetypeFont::codepdata &TruetypeFont::GetTexFromCodepoint(int cp)
{
    if (Texes->find(cp) == Texes->end())
    {
        codepdata newcp;
        int w, h, xofs, yofs;
#ifdef VERBOSE_DEBUG
        wprintf(L"generating %d\n", cp);
#endif
        if (IsValid)
        {
			unsigned char* nonsdf = stbtt_GetCodepointBitmap(info.get(), 0, realscale, cp, &w, &h, &xofs, &yofs);
			if (nonsdf) {
				unsigned char* sdf = new unsigned char[w * h];

				// convert our non-sdf texture to a SDF texture
				ConvertToSDF(sdf, nonsdf, w, h);

				// free our non-sdf
				free(nonsdf);

				// use our SDF texture. alpha testing is up by default
				newcp.tex = sdf;
			}
			else
				newcp.tex = NULL;

            newcp.gltx = 0;
			newcp.xofs = xofs;
            newcp.yofs = yofs;
            newcp.w = w;
            newcp.h = h;

        }
        else
            memset(&newcp, 0, sizeof(codepdata));

		Texes->insert_or_assign(cp, newcp);
        return Texes->at(cp);
    }
    else
    {
        return Texes->at(cp);
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
            codepdata &cp = GetTexFromCodepoint(*it);

            auto it_nx = it;
            ++it_nx;
            if (it_nx != itend)
            {
                float aW = stbtt_GetCodepointKernAdvance(info.get(), *it, *it_nx);
                int bW;
                stbtt_GetCodepointHMetrics(info.get(), *it, &bW, NULL);
                Out += aW * realscale + bW * realscale;
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

void TruetypeFont::GenerateFontCache(std::filesystem::path u8charin, 
	std::filesystem::path inputttf,
	std::filesystem::path out)
{
	TTFCache cache;
	TruetypeFont ttf(inputttf);

	// make sure our base path exists
	auto path = CACHE_PATH / Game::GameState::GetInstance().GetSkin();
	std::filesystem::create_directory(path);

	auto cachename = path / inputttf.replace_extension("").filename();

	std::ifstream in(u8charin, std::ios::binary);

	std::istreambuf_iterator<char> it(in.rdbuf());
	std::istreambuf_iterator<char> end;
	
	while (it != end) {
		auto ch = utf8::next(it, end);
		auto tx = ttf.GetTexFromCodepoint(ch);
		cache.SetCharacterBuffer(ch, tx.tex, tx.w * tx.h);
	}

	cache.SaveCache(out);
}
