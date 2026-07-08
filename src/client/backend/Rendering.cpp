#include <filesystem>
#include <map>
#include <rmath.h>
#include <glm.h>
#include <GL/glew.h>
#include <SDL3/SDL.h>
#include <utf8.h>
#include <stb/stb_truetype.h>
#include <text_and_file_util.h>

#include "GameWindow.h"

#include "VBO.h"
#include "Transformation.h"
#include "Rendering.h"

#include <ranges>

#include "Sprite.h"


#include "Line.h"
#include "Texture2D.h"

#include "TruetypeFont.h"
#include "BitmapFont.h"

#include "Shader.h"

#include "TextureCollection.h"

#include "../structure/Configuration.h"

constexpr ColorRGB White = { 1, 1, 1, 1 };
constexpr ColorRGB Black = { 0, 0, 0, 1 };
constexpr ColorRGB Red = { 1, 0, 0, 1 };
constexpr ColorRGB Green = { 0, 1, 0, 1 };
constexpr ColorRGB Blue = { 0, 0, 1, 1 };


namespace renderer {
	VBO* quad_buffer = nullptr;
	VBO* texture_buffer = nullptr;
	VBO* temp_texture_buffer = nullptr;
	VBO* color_buffer = nullptr;
	Texture2D* xor_tex = nullptr;

	float QuadPositions[8] =
	{
		// tr
		1, 0,
		// br
		1, 1,
		// bl
		0, 1,
		// tl
		0, 0,
	};

	float QuadColours[16] =
	{
		// R G B A
		1, 1, 1, 1,
		1, 1, 1, 1,
		1, 1, 1, 1,
		1, 1, 1, 1
	};

	void do_quad_draw()
	{
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	}

	void draw_primitive_quad(Transformation &quad_transformation, const EBlendMode &mode, const ColorRGB &Color)
	{
		Texture2D::unbind();
		Shader::set_uniform(DefaultShader::get_uniform(U_COLOR), Color.Red, Color.Green, Color.Blue, Color.Alpha);

		set_blending_mode(mode);

		Mat4 mat = quad_transformation.GetMatrix();
		Shader::set_uniform(DefaultShader::get_uniform(U_MODELVIEW), &(mat[0][0]));

		// Assign position attrib. pointer
		set_primitive_quad_vbo();
		do_quad_draw();
		finalize_draw();

		Texture2D::force_rebind();
	}

	void set_xor_tex_parameters() {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}

	static bool Initialized = false;
	void initialize()
	{
		if (!Initialized)
		{
			quad_buffer = new VBO(VBO::Static, sizeof(QuadPositions) / sizeof(float));
			quad_buffer->assign_data(QuadPositions);
			assert(glGetError() == 0);

			texture_buffer = new VBO(VBO::Static, sizeof(QuadPositions) / sizeof(float));
			texture_buffer->assign_data(QuadPositions);
            assert(glGetError() == 0);

			temp_texture_buffer = new VBO(VBO::Static, sizeof(QuadPositions) / sizeof(float));
			temp_texture_buffer->assign_data(QuadPositions);
            assert(glGetError() == 0);

			color_buffer = new VBO(VBO::Static, sizeof(QuadColours) / sizeof(float));
			color_buffer->assign_data(QuadColours);
            assert(glGetError() == 0);

			// create xor texture
			xor_tex = new Texture2D;
			
			std::vector<uint32_t> buf(256 * 256);
			for(int i = 0; i < 256; i++) {
				for (int j = 0; j < 256; j++) {
					uint8_t x = i ^ j;

					buf[i * 256 + j] = (255 << 24) + x + (x << 8) + (x << 16);
				}
			}

			ImageData2d d(256, 256, nullptr);
			d.Data = buf;

			xor_tex->set_texture_data_2d(d);
            assert(glGetError() == 0);

			xor_tex->fname = "xor";

			set_xor_tex_parameters(); // it's bound by SetTextureData2D, apply parameters
            assert(glGetError() == 0);

			TextureCollection::register_texture(xor_tex);
            assert(glGetError() == 0);


            Initialized = true;
		}
	}

	Texture2D* get_xor_texture(){
		return xor_tex;
	}

	

	void set_texture_parameters(std::string Dir)
	{
		auto wrapS = GL_CLAMP_TO_EDGE, wrapT = GL_CLAMP_TO_EDGE;
		if (Configuration::GetTextureParameter(Dir, "wrap-s") == "clamp-edge")
			wrapS = GL_CLAMP_TO_EDGE;
		else if (Configuration::GetTextureParameter(Dir, "wrap-s") == "repeat")
			wrapS = GL_REPEAT;
		else if (Configuration::GetTextureParameter(Dir, "wrap-s") == "clamp-border")
			wrapS = GL_CLAMP_TO_BORDER;
		else if (Configuration::GetTextureParameter(Dir, "wrap-s") == "repeat-mirrored")
			wrapS = GL_MIRRORED_REPEAT;

		if (Configuration::GetTextureParameter(Dir, "wrap-t") == "clamp-edge")
			wrapT = GL_CLAMP_TO_EDGE;
		else if (Configuration::GetTextureParameter(Dir, "wrap-t") == "repeat")
			wrapT = GL_REPEAT;
		else if (Configuration::GetTextureParameter(Dir, "wrap-t") == "clamp-border")
			wrapT = GL_CLAMP_TO_BORDER;
		else if (Configuration::GetTextureParameter(Dir, "wrap-t") == "repeat-mirrored")
			wrapT = GL_MIRRORED_REPEAT;

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
        assert (glGetError() == 0);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);
        assert (glGetError() == 0);


        GLint minp = GL_LINEAR, maxp = GL_LINEAR;
        bool mipmaps = false;
		if (Configuration::GetTextureParameter(Dir, "minfilter") == "linear")
			minp = GL_LINEAR;
		else if (Configuration::GetTextureParameter(Dir, "minfilter") == "nearest")
			minp = GL_NEAREST;
		else if (Configuration::GetTextureParameter(Dir, "minfilter") == "linear-mipmap-linear") {
            minp = GL_LINEAR_MIPMAP_LINEAR;
            mipmaps = true;
        }
		else if (Configuration::GetTextureParameter(Dir, "minfilter") == "linear-mipmap-nearest") {
            minp = GL_LINEAR_MIPMAP_NEAREST;
            mipmaps = true;
        }
		else if (Configuration::GetTextureParameter(Dir, "minfilter") == "nearest-mipmap-nearest") {
			minp = GL_NEAREST_MIPMAP_NEAREST;
		    mipmaps = true;
		}
		else if (Configuration::GetTextureParameter(Dir, "minfilter") == "nearest-mipmap-linear") {
            minp = GL_NEAREST_MIPMAP_LINEAR;
            mipmaps = true;
        }

		if (Configuration::GetTextureParameter(Dir, "maxfilter") == "linear")
			maxp = GL_LINEAR;
		else if (Configuration::GetTextureParameter(Dir, "maxfilter") == "nearest")
			maxp = GL_NEAREST;
		else if (Configuration::GetTextureParameter(Dir, "maxfilter") == "linear-mipmap-linear") {
			maxp = GL_LINEAR_MIPMAP_LINEAR;
		    mipmaps = true;
		}
		else if (Configuration::GetTextureParameter(Dir, "maxfilter") == "linear-mipmap-nearest") {
			maxp = GL_LINEAR_MIPMAP_NEAREST;
		    mipmaps = true;
		}
		else if (Configuration::GetTextureParameter(Dir, "maxfilter") == "nearest-mipmap-nearest") {
			maxp = GL_NEAREST_MIPMAP_NEAREST;
		    mipmaps = true;
		}
		else if (Configuration::GetTextureParameter(Dir, "maxfilter") == "nearest-mipmap-linear") {
			maxp = GL_NEAREST_MIPMAP_LINEAR;
		    mipmaps = true;
		}

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minp);
        assert (glGetError() == 0);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, maxp);
        assert (glGetError() == 0);

        if (mipmaps) {
            glGenerateMipmap(GL_TEXTURE_2D);
            assert (glGetError() == 0);
        }
    }

	void set_blending_mode(const EBlendMode mode)
	{
		if (mode == BLEND_ADD)
		{
			glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		}
		else if (mode == BLEND_ALPHA)
		{
			glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}
	}

	void set_primitive_quad_vbo()
	{
		quad_buffer->bind();
		glVertexAttribPointer(Shader::enable_attrib_array(DefaultShader::get_uniform(A_POSITION)), 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, nullptr);
		glVertexAttribPointer(Shader::enable_attrib_array(DefaultShader::get_uniform(A_UV)), 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, nullptr);
		color_buffer->bind();
		glVertexAttribPointer(Shader::enable_attrib_array(DefaultShader::get_uniform(A_COLOR)), 4, GL_FLOAT, GL_FALSE, sizeof(float) * 4, nullptr);
	}

	void set_textured_quad_vbo(VBO *tex_quad)
	{
		quad_buffer->bind();
		glVertexAttribPointer(Shader::enable_attrib_array(DefaultShader::get_uniform(A_POSITION)), 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, nullptr);
		tex_quad->bind();
		glVertexAttribPointer(Shader::enable_attrib_array(DefaultShader::get_uniform(A_UV)), 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, nullptr);
		color_buffer->bind();
		glVertexAttribPointer(Shader::enable_attrib_array(DefaultShader::get_uniform(A_COLOR)), 4, GL_FLOAT, GL_FALSE, sizeof(float) * 4, nullptr);
	}

	void finalize_draw()
	{
		Shader::disable_attrib_array(DefaultShader::get_uniform(A_POSITION));
		Shader::disable_attrib_array(DefaultShader::get_uniform(A_UV));
		Shader::disable_attrib_array(DefaultShader::get_uniform(A_COLOR));
	}

	void set_default_shader_parameters(const bool invert_color,
                               const bool centered,
                               const bool black_to_transparent, const bool replace_color,
                               const int8_t hidden_mode)
	{
		DefaultShader::static_bind();
		Shader::set_uniform(DefaultShader::get_uniform(U_INVERT), invert_color);

		if (hidden_mode == -1)
			Shader::set_uniform(DefaultShader::get_uniform(U_HIDDEN), 0); // not affected by hidden lightning
		else
			Shader::set_uniform(DefaultShader::get_uniform(U_HIDDEN), hidden_mode); // Assume the other related parameters are already set.

		Shader::set_uniform(DefaultShader::get_uniform(U_REPCOLOR), replace_color);
		Shader::set_uniform(DefaultShader::get_uniform(U_BTRANSP), black_to_transparent);

		Shader::set_uniform(DefaultShader::get_uniform(U_CENTERED), centered);
	}

	void draw_quad(const QuadDrawParams &params)
	{
		set_blending_mode(params.blend_mode);
		assert(glGetError() == 0);

		const auto color = params.color;

		if (!params.shader) {
			if (params.configure_default_shader) {
				set_default_shader_parameters(
					params.invert_color,
					params.centered,
					params.black_to_transparent,
					params.replace_color,
					params.hidden_mode
				);
				assert(glGetError() == 0);
			}

			DefaultShader::set_color(color.Red, color.Green, color.Blue, color.Alpha);
			assert(glGetError() == 0);

			if (params.model) {
				Shader::set_uniform(DefaultShader::get_uniform(U_MODELVIEW), &((*params.model)[0][0]));
				assert(glGetError() == 0);
			}
		} else {
			const auto projection = window.get_matrix_projection();
			params.shader->bind();

			auto uniform = params.shader->get_uniform("projection");
			assert(glGetError() == 0);
			if (uniform != -1)
				Shader::set_uniform(uniform, &projection[0][0]);
			assert(glGetError() == 0);

			uniform = params.shader->get_uniform("mvp");
			assert(glGetError() == 0);
			if (uniform != -1 && params.model)
				Shader::set_uniform(uniform, &((*params.model)[0][0]));
			assert(glGetError() == 0);

			uniform = params.shader->get_uniform("centered");
			assert(glGetError() == 0);
			if (uniform != -1)
				Shader::set_uniform(uniform, params.centered);
			assert(glGetError() == 0);

			uniform = params.shader->get_uniform("color");
			assert(glGetError() == 0);
			if (uniform != -1)
				Shader::set_uniform(
					uniform,
					l2gamma(color.Red),
					l2gamma(color.Green),
					l2gamma(color.Blue),
					color.Alpha
				);
			assert(glGetError() == 0);
		}

		const auto texture_coordinates = params.texture_coordinates ? params.texture_coordinates : texture_buffer;
		if (params.configure_geometry) {
			set_textured_quad_vbo(texture_coordinates);
		} else {
			texture_coordinates->bind();
			glVertexAttribPointer(
				Shader::enable_attrib_array(DefaultShader::get_uniform(A_UV)),
				2,
				GL_FLOAT,
				GL_FALSE,
				sizeof(float) * 2,
				nullptr
			);
		}
		assert(glGetError() == 0);

		do_quad_draw();
		assert(glGetError() == 0);

		if (params.finalize) {
			finalize_draw();
			assert(glGetError() == 0);
		}
	}

	void draw_textured_quad(Texture2D* to_draw, const AABB& texture_crop, const Transformation& quad_transformation,
		const EBlendMode &mode, const ColorRGB &in_color)
	{
		if (to_draw)
			to_draw->bind();
		else return;

		Shader::set_uniform(DefaultShader::get_uniform(U_COLOR), in_color.Red, in_color.Green, in_color.Blue, in_color.Alpha);

		set_blending_mode(mode);

		float CropPositions[8] = {
			// topright
			texture_crop.P2.X / float(to_draw->w),
			texture_crop.P2.Y / float(to_draw->h),
			// bottom right
			texture_crop.P2.X / float(to_draw->w),
			texture_crop.P1.Y / float(to_draw->h),
			// bottom left
			texture_crop.P1.X / float(to_draw->w),
			texture_crop.P1.Y / float(to_draw->h),
			// topleft
			texture_crop.P1.X / float(to_draw->w),
			texture_crop.P2.Y / float(to_draw->h),
		};

		temp_texture_buffer->assign_data(CropPositions);
		set_textured_quad_vbo(temp_texture_buffer);

		do_quad_draw();

		finalize_draw();
	}
	
	VBO* get_default_geometry_buffer()
	{
		return quad_buffer;
	}
	
	VBO* get_default_texture_buffer()
	{
		return texture_buffer;
	}

	VBO* get_default_color_buffer()
	{
		return color_buffer;
	}

	void set_current_object_matrix(glm::mat4 &mat)
	{
	    Shader::set_uniform(DefaultShader::get_uniform(U_MODELVIEW), &(mat[0][0]));
	}

	void set_scissor(const bool enabled)
	{
		if (enabled) glEnable(GL_SCISSOR_TEST);
		else glDisable(GL_SCISSOR_TEST);
	}

	void set_scissor_region(const int x, const int y, const int w, const int h) {
		float ratio = window.get_window_v_scale();
		glScissor(x * ratio, (ScreenHeight - (y + h)) * ratio, w * ratio, h * ratio);
	}

	void set_scissor_region_wnd(const int x, const int y, const int w, const int h) {
		// x: 0 -> 0; screenwidth -> windowwidth
		// y: 0 -> windowheight; screenheight -> 0
		auto tx = [&](const int x) {
			auto ww = window.get_window_size().x;
			return x * ww / ScreenWidth;
		};

		auto ty = [&](const int y) {
			auto wh = window.get_window_size().y;
			auto m = -wh / ScreenHeight;
			return m * y + wh;
		};

		auto vratio = window.get_window_size().y / ScreenHeight;
		glScissor(tx(x), ty(y) - h * vratio, tx(w), h * vratio);
	}
}

void Sprite::update_texture()
{
	if (!renderer::Initialized) return;

    if (!dirty_texture_)
        return;

    if (!do_texture_cleanup_) // We must not own a UV buffer.
    {
        dirty_texture_ = false;
        return;
    }

    if (!uv_buffer_)
        uv_buffer_ = new VBO(VBO::Dynamic, 8);

    uv_buffer_->validate();

    float CropPositions[8] = { // 2 for each vertex and a uniform for z order
		// topright
		mCrop_x2,
		mCrop_y1,
		// bottom right
		mCrop_x2,
		mCrop_y2,
		// bottom left
		mCrop_x1,
		mCrop_y2,
		// topleft
		mCrop_x1,
		mCrop_y1,
    };

    uv_buffer_->assign_data(CropPositions);
    dirty_texture_ = false;
}

bool Sprite::should_draw() const
{
    if (alpha == 0)
        return false;

	if (m_shader_)
		if (!m_shader_->is_valid())
			return false;

    if (m_texture_)
    {
		m_texture_->bind();
		return m_texture_->is_bound();
    }
    else
        return m_shader_ && m_shader_->is_valid();

    return true;
}

bool Sprite::render_minimal_setup()
{
    if (!should_draw())
        return false;

//    Renderer::SetScissor(Scissor);
//    Renderer::SetScissorRegion(ScissorRegion.X1, ScissorRegion.Y1, ScissorRegion.width(), ScissorRegion.height());

    update_texture();

	auto quad_color = color;
	quad_color.Alpha = alpha;

	renderer::QuadDrawParams params;
	params.texture_coordinates = uv_buffer_;
	params.blend_mode = blending_mode_;
	params.color = quad_color;
	params.configure_default_shader = false;
	params.configure_geometry = false;
	params.finalize = false;

	renderer::draw_quad(params);

    return true;
}

void Sprite::render()
{
    if (!should_draw())
        return;

    renderer::set_scissor(scissor);
    renderer::set_scissor_region(scissor_region.X1, scissor_region.Y1, scissor_region.width(), scissor_region.height());

    update_texture();
    assert(glGetError() == 0);

	auto mat = GetMatrix();
	auto quad_color = color;
	quad_color.Alpha = alpha;

	renderer::QuadDrawParams params;
	params.texture_coordinates = uv_buffer_;
	params.model = &mat;
	params.shader = m_shader_;
	params.blend_mode = blending_mode_;
	params.color = quad_color;
	params.centered = centered;
	params.invert_color = color_invert;
	params.black_to_transparent = black_to_transparent;

	renderer::draw_quad(params);
    assert(glGetError() == 0);
}

void Sprite::cleanup()
{
    if (do_texture_cleanup_)
    {
        delete uv_buffer_;
		uv_buffer_ = nullptr;
    }
}

void TruetypeFont::release_codepoint(const int cp) const
{
    if (Texes->contains(cp))
    {
        free(Texes->at(cp).tex);
        glDeleteTextures(1, &Texes->at(cp).gltx);
        Texes->erase(cp);
    }
}

void TruetypeFont::render(const std::string &in, const Vec2 &position, const Mat4 &transform, const Vec2 &scale)
{
    const char* Text = in.c_str();
    int Line = 0;
	size_t len = in.length();
    glm::vec3 vOffs(position.x, position.y + scale.y, 0);

    if (!IsValid)
        return;


//    Renderer::SetScissor(Scissor);
//    Renderer::SetScissorRegion(ScissorRegion.X1, ScissorRegion.Y1, ScissorRegion.width(), ScissorRegion.height());

    renderer::DefaultShader::static_bind();
    renderer::set_blending_mode(BLEND_ALPHA);
    renderer::set_default_shader_parameters(false, false, false, true);
    renderer::DefaultShader::set_color(Red, Green, Blue, Alpha);
    renderer::set_primitive_quad_vbo();

    try
    {
		auto nd = utf8::find_invalid<const char*>(Text, Text + in.length());
        utf8::iterator<const char*> it(Text, Text, nd);
        utf8::iterator<const char*> itend(nd, Text, nd);
        for (; it != itend; ++it)
        {
            codepdata &cp = GetTexFromCodepoint(*it);
            unsigned char* tx = cp.tex;
            glm::vec3 trans = vOffs + glm::vec3(
				cp.xofs * scale.x * scale.y / SDF_SIZE,
				cp.yofs * scale.y / SDF_SIZE, 0);
            glm::mat4 dx;

            if (*it == 10) // utf-32 line feed
            {
                Line++;
                vOffs.x = position.x;
				vOffs.y = position.y + (Line + 1) * scale.y;
                continue;
            }

            if(!tx || !cp.w || !cp.h)
                goto advance;

            // do the actual draw?
            if (cp.gltx == 0)
            {
                glGenTextures(1, &cp.gltx);
                glBindTexture(GL_TEXTURE_2D, cp.gltx);

                glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

				// SDF texture => filtering
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, cp.w, cp.h, 0, GL_RED, GL_UNSIGNED_BYTE, tx);
            }
            else if (cp.gltx)
                glBindTexture(GL_TEXTURE_2D, cp.gltx);

            dx = transform * glm::translate(glm::identity<Mat4>(), trans) *
                 glm::scale(glm::identity<Mat4>(), glm::vec3(cp.w * scale.y / SDF_SIZE, cp.h * scale.y / SDF_SIZE, 1));

            renderer::Shader::set_uniform(renderer::DefaultShader::get_uniform(renderer::U_MODELVIEW), &(dx[0][0]));

            renderer::do_quad_draw();

            advance:
            utf8::iterator<const char*> next = it;
            next++;
            if (next != itend)
            {
                float aW = stbtt_GetCodepointKernAdvance(info.get(), *it, *next);
                int bW;
                stbtt_GetCodepointHMetrics(info.get(), *it, &bW, nullptr);
                vOffs.x += (aW * realscale + bW * realscale) * scale.x  * scale.y / SDF_SIZE;
            }
        }
    }
#ifndef NDEBUG
    catch (utf8::exception &ex)
    {
        otoworm::util::debug_break();
        //Log::Logf("Invalid UTF-8 string %s was passed. Error type: %s\n", ex.what());
    }
#else
    catch (...)
    {
        // nothing
    }
#endif

    renderer::finalize_draw();
    Texture2D::force_rebind();
}

void TruetypeFont::release_textures() const
{
    for (auto &val: *Texes | std::views::values)
    {
		if (val.tex) {
			free(val.tex);
			val.tex = nullptr;
		}
		if (val.gltx) {
			glDeleteTextures(1, &val.gltx);
			val.gltx = 0;
		}
    }
}

void Line::update_vbo()
{
    if (NeedsUpdate)
    {
        if (!lnvbo)
        {
            lnvbo = new VBO(VBO::Stream, 4);
        }

        lnvbo->validate();

        float xx[] = {
            x1, y1,
            x2, y2
        };

        lnvbo->assign_data(xx);

        NeedsUpdate = false;
    }
}

void Line::render()
{
    auto Identity = glm::identity<Mat4>();
    update_vbo();

    glDisable(GL_DEPTH_TEST);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Set the color.
	using namespace renderer;
	set_default_shader_parameters(true, false, false, false);

    DefaultShader::set_color(R, G, B, A);
    Shader::set_uniform(DefaultShader::get_uniform(U_MODELVIEW), &(Identity[0][0]));

    // Assign position attrib. pointer
    lnvbo->bind();
    glVertexAttribPointer(Shader::enable_attrib_array(DefaultShader::get_uniform(A_POSITION)), 2, GL_FLOAT, GL_FALSE, 0, nullptr);

	color_buffer->bind();
    glVertexAttribPointer(Shader::enable_attrib_array(DefaultShader::get_uniform(A_COLOR)), 4, GL_FLOAT, GL_FALSE, sizeof(float) * 4, nullptr);

    glDrawArrays(GL_LINES, 0, 2);

    Shader::disable_attrib_array(DefaultShader::get_uniform(A_POSITION));
    Shader::disable_attrib_array(DefaultShader::get_uniform(A_COLOR));

    Texture2D::force_rebind();

    glEnable(GL_DEPTH_TEST);
}

void BitmapFont::render(const std::string &In, const Vec2 &Position, const Mat4 &Transform, const Vec2 &Scale)
{
    const char* Text = In.c_str();
    int32_t Character = 0, Line = 0;
    /* OpenGL Code Ahead */

    if (!Font)
        return;

    if (!Font->is_valid_)
    {
        for (int i = 0; i < 256; i++)
        {
            CharPosition[i].invalidate();
            CharPosition[i].initialize(true);
        }
        Font->is_valid_ = true;
    }

//    Renderer::SetScissor(Scissor);
//    Renderer::SetScissorRegion(ScissorRegion.X1, ScissorRegion.Y1, ScissorRegion.width(), ScissorRegion.height());

	using namespace renderer;
	set_default_shader_parameters(false, false);
    DefaultShader::set_color(Red, Green, Blue, Alpha);

    Font->bind();

    // Assign position attrib. pointer
	renderer::quad_buffer->bind();
    glVertexAttribPointer(Shader::enable_attrib_array(DefaultShader::get_uniform(A_POSITION)), 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, nullptr);

	renderer::color_buffer->bind();
    glVertexAttribPointer(Shader::enable_attrib_array(DefaultShader::get_uniform(A_COLOR)), 4, GL_FLOAT, GL_FALSE, sizeof(float) * 4, nullptr);

    for (; *Text != '\0'; Text++)
    {
        if (*Text == '\n')
        {
            Character = 0;
            Line += RenderSize.y;
            continue;
        }

        if (*Text < 0)
            continue;

        CharPosition[*Text].set_position(Position.x + Character, Position.y + Line);
        Mat4 RenderTransform = Transform * CharPosition[*Text].GetMatrix();

        // Assign transformation matrix
        Shader::set_uniform(DefaultShader::get_uniform(U_MODELVIEW), &(RenderTransform[0][0]));

        // Assign vertex UVs
        CharPosition[*Text].bind_texture_vbo();
        glVertexAttribPointer(Shader::enable_attrib_array(DefaultShader::get_uniform(A_UV)), 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, nullptr);

        // Do the rendering!
        renderer::do_quad_draw();

        Shader::disable_attrib_array(DefaultShader::get_uniform(A_UV));

        Character += RenderSize.x;
    }

    Shader::disable_attrib_array(DefaultShader::get_uniform(A_POSITION));
    Shader::disable_attrib_array(DefaultShader::get_uniform(A_COLOR));
}

uint32_t VBO::LastBound = 0;
uint32_t VBO::LastBoundIndex = 0;

VBO::VBO(const Type T, const uint32_t Elements, const uint32_t Size, const IdxKind Kind)
{
    InternalVBO = 0;
    IsValid = false;
    mType = T;
    mKind = Kind;
    window.add_vbo(this);

    ElementCount = Elements;
    ElementSize = Size;
    VboData = new char[ElementSize * ElementCount];
}

VBO::~VBO()
{
    if (InternalVBO)
    {
        glDeleteBuffers(1, &InternalVBO);
        InternalVBO = 0;
    }

    window.remove_vbo(this);

    delete[] VboData;
    VboData = nullptr;
}

uint32_t VBO::get_element_count() const
{
    return ElementCount;
}

void VBO::invalidate()
{
    IsValid = false;
}

void VBO::validate()
{
    if (!IsValid)
        assign_data(VboData);
}

unsigned int up_type_for_kind(const VBO::Type mType)
{
    auto UpType = 0;

    if (mType == VBO::Stream)
        UpType = GL_STREAM_DRAW;
    else if (mType == VBO::Dynamic)
        UpType = GL_DYNAMIC_DRAW;
    else if (mType == VBO::Static)
        UpType = GL_STATIC_DRAW;

    return UpType;
}

unsigned int BufTypeForKind(const VBO::IdxKind mKind)
{
    unsigned int BufType = GL_ARRAY_BUFFER;

    if (mKind == VBO::ArrayBuffer)
        BufType = GL_ARRAY_BUFFER;
    else if (mKind == VBO::IndexBuffer)
        BufType = GL_ELEMENT_ARRAY_BUFFER;
    return BufType;
}

void VBO::assign_data(const void* Data)
{
    bool RegenBuffer = false;

    const unsigned int up_type = up_type_for_kind(mType);
    const unsigned int buf_type = BufTypeForKind(mKind);

    memmove(VboData, Data, ElementSize * ElementCount);

    if (!IsValid)
    {
        glGenBuffers(1, &InternalVBO);
        IsValid = true;
        RegenBuffer = true;
    }

    bind(true);
    if (RegenBuffer)
        glBufferData(buf_type, ElementSize * ElementCount, VboData, up_type);
    else
        glBufferSubData(buf_type, 0, ElementSize * ElementCount, VboData);
}

void VBO::bind(const bool force) const
{
    assert(IsValid);

    if (mKind == ArrayBuffer && (LastBound != InternalVBO || force))
    {
        glBindBuffer(BufTypeForKind(mKind), InternalVBO);
        LastBound = InternalVBO;
    }
    else if (mKind == IndexBuffer && (LastBoundIndex != InternalVBO || force))
    {
        glBindBuffer(BufTypeForKind(mKind), InternalVBO);
        LastBoundIndex = InternalVBO;
    }
}
