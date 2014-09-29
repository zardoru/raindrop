
#include <map>

struct stbtt_fontinfo;
class VBO;

class TruetypeFont
{
	stbtt_fontinfo* info;
	unsigned char* data;
	size_t offs;
	uint32 tex;
	bool IsValid;
	float scale;
	float realscale;

	struct codepdata
	{
		unsigned char* tex;
		uint32 gltx;
		int xofs;
		int yofs;
		int w;
		int h;
	};
	
	std::map<int, codepdata> Texes;
	VBO *Texform;
	void SetupTexture();
	codepdata& GetTexFromCodepoint(int cp);

public:
	TruetypeFont(Directory Filename, float Scale);
	~TruetypeFont();
	void Invalidate();
	void Render(const char* Text, const Vec2 &Position);
};