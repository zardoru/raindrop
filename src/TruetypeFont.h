
#include "Font.h"
#include <map>

struct stbtt_fontinfo;
class VBO;

class TruetypeFont : public Font
{
	stbtt_fontinfo* info;
	unsigned char* data;
	size_t offs;
	bool IsValid;
	float scale;
	float realscale;

	float windowscale;

	struct codepdata
	{
		unsigned char* tex;
		uint32 gltx;
		int xofs;
		int yofs;
		int w;
		int h;
		float scl;
	};
	
	std::map<int, codepdata> Texes;
	VBO *Texform;
	void SetupTexture();
	codepdata& GetTexFromCodepoint(int cp);
	void CheckCodepoint(int cp);
	void ReleaseCodepoint(int cp);
	void ReleaseTextures();
	void UpdateWindowScale();

public:
	TruetypeFont(Directory Filename, float Scale);
	~TruetypeFont();
	void Invalidate();
	void Render(GString Text, const Vec2 &Position, const Mat4 &Transform = Mat4());
};
