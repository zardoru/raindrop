#pragma once

#include "DrawCallSink.h"

class VBO;

class Line
{
    VBO *lnvbo;

    // Do NOT change the order of this. UpdateVBO depends on that.
    float x1, y1, x2, y2;

    float R, G, B, A;

    bool NeedsUpdate;
    void update_vbo();
public:
    Line();

    void set_color(float R, float G, float B, float A);
    void set_location(const Vec2 &p1, const Vec2 &p2);
    void emit_draw_calls(DrawCallSink &sink, uint32_t z = 0) const;
};
