#pragma once

class Font;

class GraphicalString : public Sprite
{
    std::string mText;
    Font* mFont;
public:
    GraphicalString();
    void SetText(std::string _Text);
    std::string GetText() const;
    void SetFont(Font* _Font);
    Font* GetFont() const;

    void Render() override;
};
