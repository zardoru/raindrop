#ifndef LINE_H_
#define LINE_H_

class VBO;

class Line 
{
	VBO *lnvbo;

	// Do NOT change the order of this. UpdateVBO depends on that.
	float x1, y1, x2, y2;

	float R, G, B, A;

	bool NeedsUpdate;
	void UpdateVBO();
public:
	Line();

	void SetColor(float _R, float _G, float _B, float _A);
	void SetLocation(const Vec2 &p1, const Vec2 &p2);
	void Render();
};

#endif