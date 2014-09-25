
struct stbtt_fontinfo;
class VBO;

class TruetypeFont
{
	stbtt_fontinfo* info;
	unsigned char* data;
	size_t offs;
	uint32 tex;

	VBO *Texform;
	void SetupTexture();
public:
	TruetypeFont(Directory Filename);

	void Render(const char* Text, const Vec2 &Position, const float &Scale);
};