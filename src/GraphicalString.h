#pragma once

class Font;

class GraphicalString : public Sprite
{
    std::string mText;
    Font* mFont;
	float mFontHeight;
	float mKernScale;
public:
    GraphicalString();
    void SetText(std::string _Text);
    std::string GetText() const;
    void SetFont(Font* _Font);
    Font* GetFont() const;

	float GetKerningScale() const;
	void SetKerningScale(float ks);
	float GetTextSize() const;
	void SetFontSize(float fsize);
	float GetFontSize() const;

    void Render() override;
};
