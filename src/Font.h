#ifndef FONT_H_
#define FONT_H_

class Font {
protected:
	float Red, Green, Blue, Alpha;
public:
	Font();

	void SetColor(float _Red, float _Green, float _Blue);
	void SetAlpha(float _Alpha);

	virtual float GetHorizontalLength(const char *Text);
	virtual void Invalidate();
	virtual void Render(const GString &Text, const Vec2 &Position, const Mat4& Transform = Mat4());
};

#endif