#ifndef RENDERING_H_
#define RENDERING_H_

#include "Global.h"
#include "Transformation.h"

enum RBlendMode
{
	MODE_ADD,
	MODE_ALPHA,
	MODE_MULTIPLY
};

class Image;
class VBO;
extern VBO* QuadBuffer;
extern VBO* TextureBuffer;

void InitializeRender();
void SetShaderParameters(bool InvertColor, 
	bool UseGlobalLight, bool Centered, bool UseSecondTransformationMatrix = false, 
	bool BlackToTransparent = false, bool ReplaceColor = false,
	int8 HiddenMode = -1);

void SetPrimitiveQuadVBO();
void FinalizeDraw();
void DoQuadDraw();
void SetBlendingMode(RBlendMode Mode);
void SetTexturedQuadVBO(VBO *TexQuad);
void DrawTexturedQuad(Image* ToDraw, const AABB& TextureCrop, const Transformation& QuadTransformation, const RBlendMode &Mode = MODE_ALPHA, const ColorRGB &InColor = Color::White);
void DrawPrimitiveQuad(Transformation &QuadTransformation, const RBlendMode &Mode = MODE_ALPHA, const ColorRGB &InColor = Color::White);

#endif