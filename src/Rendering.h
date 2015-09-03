#ifndef RENDERING_H_
#define RENDERING_H_

#include "Global.h"
#include "Transformation.h"

enum EBlendMode
{
	BLEND_ADD,
	BLEND_ALPHA,
	BLEND_MULTIPLY
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
void SetBlendingMode(EBlendMode Mode);
void SetTexturedQuadVBO(VBO *TexQuad);
void DrawTexturedQuad(Image* ToDraw, const AABB& TextureCrop, const Transformation& QuadTransformation, const EBlendMode &Mode = BLEND_ALPHA, const ColorRGB &InColor = Color::White);
void DrawPrimitiveQuad(Transformation &QuadTransformation, const EBlendMode &Mode = BLEND_ALPHA, const ColorRGB &InColor = Color::White);

#endif