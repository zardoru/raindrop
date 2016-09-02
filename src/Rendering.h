#pragma once

#include "Transformation.h"

enum EBlendMode
{
    BLEND_ADD,
    BLEND_ALPHA,
    BLEND_MULTIPLY
};

class Texture;
class VBO;

namespace Renderer {
	void InitializeRender();
	void SetShaderParameters(bool InvertColor,
		bool UseGlobalLight, bool Centered, bool UseSecondTransformationMatrix = false,
		bool BlackToTransparent = false, bool ReplaceColor = false,
		int8_t HiddenMode = -1);

	void SetTextureParameters(std::string param_src);

	void SetPrimitiveQuadVBO();
	void FinalizeDraw();
	void DoQuadDraw();
	void SetBlendingMode(EBlendMode Mode);
	void SetTexturedQuadVBO(VBO *TexQuad);
	void DrawTexturedQuad(Texture* ToDraw, const AABB& TextureCrop, const Transformation& QuadTransformation, const EBlendMode &Mode = BLEND_ALPHA, const ColorRGB &InColor = Color::White);
	void DrawPrimitiveQuad(Transformation &QuadTransformation, const EBlendMode &Mode = BLEND_ALPHA, const ColorRGB &InColor = Color::White);

	VBO* GetDefaultGeometryBuffer();
	VBO* GetDefaultTextureBuffer();
	VBO* GetDefaultColorBuffer();
}