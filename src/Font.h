#pragma once

class Font
{
protected:
    float Red, Green, Blue, Alpha;
public:
    Font();

    void SetColor(float _Red, float _Green, float _Blue);
    void SetAlpha(float _Alpha);

    virtual float GetHorizontalLength(const char *Text);
    virtual void Invalidate();
    virtual void Render(const std::string &Text, const Vec2 &Position, const Mat4& Transform = Mat4());
};