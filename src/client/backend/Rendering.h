#pragma once

#include <rmath.h>
#include <Transformation.h>

class Texture2D;
class VBO;

namespace renderer {
	class Shader;

	struct QuadDrawParams {
		VBO *texture_coordinates = nullptr;
		const Mat4 *model = nullptr;
		Shader *shader = nullptr;
		EBlendMode blend_mode = BLEND_ALPHA;
		ColorRGBA color = { 1, 1, 1, 1 };
		bool centered = false;
		bool invert_color = false;
		bool black_to_transparent = false;
		bool replace_color = false;
		int8_t hidden_mode = -1;
		bool configure_default_shader = true;
		bool configure_geometry = true;
		bool finalize = true;
	};

	void initialize();
	void set_default_shader_parameters(bool invert_color,
                               bool centered,
                               bool black_to_transparent = false, bool replace_color = false,
                               int8_t hidden_mode = -1);

	void set_texture_parameters(std::string param_src);

	void set_primitive_quad_vbo();
	void finalize_draw();
	void do_quad_draw();
	void set_blending_mode(EBlendMode mode);
	void set_textured_quad_vbo(VBO *tex_quad);
	void draw_quad(const QuadDrawParams &params = {});
	void draw_textured_quad(Texture2D* to_draw, const AABB& texture_crop, const Transformation& quad_transformation, const EBlendMode &mode = BLEND_ALPHA, const ColorRGBA &in_color = Color::white);
	void draw_primitive_quad(Transformation &quad_transformation, const EBlendMode &mode = BLEND_ALPHA, const ColorRGBA &in_color = Color::white);

	void set_scissor(bool enable);
	void set_scissor_region(int x, int y, int w, int h);
	void set_scissor_region_wnd(int x, int y, int w, int h);

	VBO* get_default_geometry_buffer();
	VBO* get_default_texture_buffer();
	VBO* get_default_color_buffer();

	Texture2D* get_xor_texture();
}

inline float l2gamma (const float c) {
	/*if (c > 1.) return 1.;
	else if (c < 0.) return 0.;
	else */
	if (c <= 0.04045) return c / 12.92;
	else return pow((c + 0.055) / 1.055, 2.4);
};
