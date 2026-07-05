#pragma once

#include <rmath.h>
#include <Transformation.h>

class Texture;
class VBO;

namespace renderer {
	void initialize();
	void set_default_shader_parameters(bool InvertColor,
                               bool Centered,
                               bool BlackToTransparent = false, bool ReplaceColor = false,
                               int8_t HiddenMode = -1);

	void set_texture_parameters(std::string param_src);

	void set_primitive_quad_vbo();
	void finalize_draw();
	void do_quad_draw();
	void set_blending_mode(EBlendMode Mode);
	void set_textured_quad_vbo(VBO *TexQuad);
	void draw_textured_quad(Texture* ToDraw, const AABB& TextureCrop, const Transformation& QuadTransformation, const EBlendMode &Mode = BLEND_ALPHA, const ColorRGB &InColor = Color::White);
	void draw_primitive_quad(Transformation &QuadTransformation, const EBlendMode &Mode = BLEND_ALPHA, const ColorRGB &InColor = Color::White);

	void set_scissor(bool enable);
	void set_scissor_region(int x, int y, int w, int h);
	void set_scissor_region_wnd(int x, int y, int w, int h);

	VBO* get_default_geometry_buffer();
	VBO* get_default_texture_buffer();
	VBO* get_default_color_buffer();

	Texture* get_xor_texture();
}

inline float l2gamma (const float c) {
	/*if (c > 1.) return 1.;
	else if (c < 0.) return 0.;
	else */
	if (c <= 0.04045) return c / 12.92;
	else return pow((c + 0.055) / 1.055, 2.4);
};