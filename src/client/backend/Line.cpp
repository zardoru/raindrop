#include <glm.h>
#include "Line.h"

Line::Line()
{
    lnvbo = NULL;
    R = G = B = A = 1;
    x1 = 0;
    x2 = 100;
    y1 = 0;
    y2 = 100;
}

void Line::SetColor(float iR, float iG, float iB, float iA)
{
    R = iR;
    G = iG;
    B = iB;
    A = iA;
}

void Line::SetLocation(const Vec2 &p1, const Vec2 &p2)
{
    x1 = p1.x;
    x2 = p2.x;

    y1 = p1.y;
    y2 = p2.y;
    NeedsUpdate = true;
}