#pragma once

#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "Font.h"
#include "Shader.h"

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
    std::unique_ptr<renderer::Shader::SDF> sdf_shader_;
    codepdata& GetTexFromCodepoint(int cp);
    void release_codepoint(int cp) const;
    void release_textures() const;
    renderer::Shader::SDF *sdf_shader();

	friend class TTFMan;
public:
    TruetypeFont(std::filesystem::path filename);
    ~TruetypeFont();
    float get_horizontal_length(const char *Text);

	static void GenerateFontCache(const std::filesystem::path& u8charin, std::filesystem::path inputttf);

    void invalidate();
    void render(const std::string &Text, const Vec2 &position, const Mat4 &transform = Mat4(), const Vec2 &scale = Vec2(1,1));
};
