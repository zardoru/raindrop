
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
	float virtualscale;
	float realscale;

	float windowscale;

	struct codepdata
	{
		unsigned char* tex;
		uint32 gltx;
		int xofs;
		int yofs;
		float scl;
		int w;
		int h;
		int tw;
		int th;
	};
	
	GString filename;
	std::map<int, codepdata> Texes;
	VBO *Texform;
	void SetupTexture();
	codepdata& GetTexFromCodepoint(int cp);
	void CheckCodepoint(int cp);
	void ReleaseCodepoint(int cp);
	void ReleaseTextures();
	void UpdateWindowScale();

public:
	TruetypeFont(GString Filename, float Scale);
	~TruetypeFont();
	float GetHorizontalLength(const char *Text);

	void Invalidate();
	void Render(const GString &Text, const Vec2 &Position, const Mat4 &Transform = Mat4());
};
