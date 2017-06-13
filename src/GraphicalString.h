#pragma once

class Font;

class GraphicalString : public Sprite
{
    std::string mText;
    Font* mFont;
	float mFontHeight;
public:
    GraphicalString();
    void SetText(std::string _Text);
    std::string GetText() const;
    void SetFont(Font* _Font);
    Font* GetFont() const;

	float GetTextSize() const;
	void SetFontSize(float fsize);
	float GetFontSize() const;

    void Render() override;
};
