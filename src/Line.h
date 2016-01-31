#pragma once

#include "Drawable.h"

class VBO;

class Line : public Drawable
{
	VBO *lnvbo;

	// Do NOT change the order of this. UpdateVBO depends on that.
	float x1, y1, x2, y2;

	float R, G, B, A;

	bool NeedsUpdate;
	void UpdateVBO();
public:
	Line();

	void SetColor(float R, float G, float B, float A);
	void SetLocation(const Vec2 &p1, const Vec2 &p2);
	void Render();
};