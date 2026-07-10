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
    void set_text(const std::string &text);
    std::string get_text() const;
    void set_font(Font* font);
    Font* get_font() const;

	float get_kerning_scale() const;
	void set_kerning_scale(float ks);
	float get_text_size() const;
	void set_font_size(float fsize);
	float get_font_size() const;

    void emit_draw_calls(DrawCallSink &sink) override;
};
