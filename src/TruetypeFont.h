#pragma once

#include "Font.h"

struct stbtt_fontinfo;
class VBO;

class TruetypeFont : public Font
{
    std::shared_ptr<stbtt_fontinfo> info;
    std::shared_ptr<std::vector<unsigned char>> data;
    size_t offs;
    bool IsValid;
    float realscale;

    struct codepdata
    {
        unsigned char* tex;
        uint32_t gltx;
        int xofs;
        int yofs;
        int w;
        int h;
    };

    std::string filename;
    std::shared_ptr<std::map<int, codepdata> > Texes;
    codepdata& GetTexFromCodepoint(int cp);
    void ReleaseCodepoint(int cp);
    void ReleaseTextures();

	friend class TTFMan;
public:
    TruetypeFont(std::filesystem::path filename);
    ~TruetypeFont();
    float GetHorizontalLength(const char *Text);

	static void GenerateFontCache(std::filesystem::path u8charin, std::filesystem::path inputttf, std::filesystem::path outpath);

    void Invalidate();
    void Render(const std::string &Text, const Vec2 &Position, const Mat4 &Transform = Mat4(), const Vec2 &Scale = Vec2(1,1));
};
