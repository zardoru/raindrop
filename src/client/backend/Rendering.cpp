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

	void draw_primitive_quad(Transformation &quad_transformation, const EBlendMode &mode, const ColorRGBA &Color)
	{
		Texture2D::unbind();
		Shader::set_uniform(Shader::Default::uniform(Shader::Default::Uniform::Color), Color.red, Color.green, Color.blue, Color.alpha);

		set_blending_mode(mode);

		Mat4 mat = quad_transformation.as_matrix();
		Shader::set_uniform(Shader::Default::uniform(Shader::Default::Uniform::ModelView), &(mat[0][0]));

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
			quad_buffer->assign(QuadPositions);
			assert(glGetError() == 0);

			texture_buffer = new VBO(VBO::Static, sizeof(QuadPositions) / sizeof(float));
			texture_buffer->assign(QuadPositions);
            assert(glGetError() == 0);

			temp_texture_buffer = new VBO(VBO::Static, sizeof(QuadPositions) / sizeof(float));
			temp_texture_buffer->assign(QuadPositions);
            assert(glGetError() == 0);

			color_buffer = new VBO(VBO::Static, sizeof(QuadColours) / sizeof(float));
			color_buffer->assign(QuadColours);
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
		glVertexAttribPointer(Shader::enable_attrib_array(A_POSITION), 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, nullptr);
		glVertexAttribPointer(Shader::enable_attrib_array(A_UV), 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, nullptr);
		color_buffer->bind();
		glVertexAttribPointer(Shader::enable_attrib_array(A_COLOR), 4, GL_FLOAT, GL_FALSE, sizeof(float) * 4, nullptr);
	}

	void set_textured_quad_vbo(VBO *tex_quad)
	{
		quad_buffer->bind();
		glVertexAttribPointer(Shader::enable_attrib_array(A_POSITION), 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, nullptr);
		tex_quad->bind();
		glVertexAttribPointer(Shader::enable_attrib_array(A_UV), 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, nullptr);
		color_buffer->bind();
		glVertexAttribPointer(Shader::enable_attrib_array(A_COLOR), 4, GL_FLOAT, GL_FALSE, sizeof(float) * 4, nullptr);
	}

	void finalize_draw()
	{
		Shader::disable_attrib_array(A_POSITION);
		Shader::disable_attrib_array(A_UV);
		Shader::disable_attrib_array(A_COLOR);
	}

	void set_default_shader_parameters(const bool centered)
	{
		Shader::Default::static_bind();
		Shader::set_uniform(Shader::Default::uniform(Shader::Default::Uniform::Centered), centered);
	}

	void draw_quad(const QuadDrawParams &params)
	{
		set_blending_mode(params.blend_mode);
		assert(glGetError() == 0);

		const auto color = params.color;

		if (!params.shader) {
			if (params.configure_default_shader) {
				set_default_shader_parameters(params.centered);
				assert(glGetError() == 0);
			}

			Shader::Default::set_color(color.red, color.green, color.blue, color.alpha);
			assert(glGetError() == 0);

			if (params.model) {
				Shader::set_uniform(Shader::Default::uniform(Shader::Default::Uniform::ModelView), &((*params.model)[0][0]));
				assert(glGetError() == 0);
			}
		} else {
			params.shader->bind();
			params.shader->apply_draw_state({GameWindow::get_instance().get_matrix_projection(), params.model, params.centered, color});
			assert(glGetError() == 0);
		}

		const auto texture_coordinates = params.texture_coordinates ? params.texture_coordinates : texture_buffer;
		if (params.configure_geometry) {
			set_textured_quad_vbo(texture_coordinates);
		} else {
			texture_coordinates->bind();
			glVertexAttribPointer(
				Shader::enable_attrib_array(A_UV),
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
		const EBlendMode &mode, const ColorRGBA &in_color)
	{
		if (to_draw)
			to_draw->bind();
		else return;

		Shader::set_uniform(Shader::Default::uniform(Shader::Default::Uniform::Color), in_color.red, in_color.green, in_color.blue, in_color.alpha);

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

		temp_texture_buffer->assign(CropPositions);
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
	    Shader::set_uniform(Shader::Default::uniform(Shader::Default::Uniform::ModelView), &(mat[0][0]));
	}

	void set_scissor(const bool enabled)
	{
		if (enabled) glEnable(GL_SCISSOR_TEST);
		else glDisable(GL_SCISSOR_TEST);
	}

	void set_scissor_region(const int x, const int y, const int w, const int h) {
		float ratio = GameWindow::get_instance().get_window_v_scale();
		glScissor(x * ratio, (ScreenHeight - (y + h)) * ratio, w * ratio, h * ratio);
	}

	void set_scissor_region_wnd(const int x, const int y, const int w, const int h) {
		// x: 0 -> 0; screenwidth -> windowwidth
		// y: 0 -> windowheight; screenheight -> 0
		auto tx = [&](const int x) {
			auto ww = GameWindow::get_instance().get_window_size().x;
			return x * ww / ScreenWidth;
		};

		auto ty = [&](const int y) {
			auto wh = GameWindow::get_instance().get_window_size().y;
			auto m = -wh / ScreenHeight;
			return m * y + wh;
		};

		auto vratio = GameWindow::get_instance().get_window_size().y / ScreenHeight;
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
		crop_.X2,
		crop_.Y1,
		// bottom right
		crop_.X2,
		crop_.Y2,
		// bottom left
		crop_.X1,
		crop_.Y2,
		// topleft
		crop_.X1,
		crop_.Y1,
    };

    uv_buffer_->assign(CropPositions);
    dirty_texture_ = false;
}

bool Sprite::should_draw(const renderer::Shader *shader) const
{
    if (color.alpha == 0)
        return false;

	if (!shader)
		shader = m_shader_;

	if (shader)
		if (!shader->is_valid())
			return false;

    if (m_texture_)
    {
		m_texture_->bind();
		return m_texture_->is_bound();
    }
    else
        return shader && shader->is_valid();

    return true;
}

void Sprite::emit_draw_calls(DrawCallSink &sink)
{
    emit_draw_calls(sink, nullptr);
}

void Sprite::emit_draw_calls(DrawCallSink &sink, renderer::Shader *shader_override)
{
    auto *shader = shader_override ? shader_override : m_shader_;
    if (!should_draw(shader))
        return;

    update_texture();

    auto matrix = as_matrix();
    renderer::QuadDrawParams params;
    params.texture_coordinates = uv_buffer_;
    params.model = &matrix;
    params.shader = shader;
    params.blend_mode = blending_mode_;
    params.color = color;
    params.centered = centered;
    sink.submit_quad(get_z(), params, m_texture_, scissor, scissor_region);
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
    const char* text = in.c_str();
    int line = 0;
    glm::vec3 v_offs(position.x, position.y + scale.y, 0);

    if (!IsValid)
        return;


//    Renderer::SetScissor(Scissor);
//    Renderer::SetScissorRegion(ScissorRegion.X1, ScissorRegion.Y1, ScissorRegion.width(), ScissorRegion.height());

    auto *shader = sdf_shader();
    if (!shader || !shader->is_valid())
        return;

    shader->bind();
    renderer::set_blending_mode(BLEND_ALPHA);
    const auto projection = GameWindow::get_instance().get_matrix_projection();
    shader->apply_draw_state({projection, nullptr, false, color_});
    renderer::set_primitive_quad_vbo();

    try
    {
		auto nd = utf8::find_invalid<const char*>(text, text + in.length());
        utf8::iterator<const char*> it(text, text, nd);
        utf8::iterator<const char*> itend(nd, text, nd);
        for (; it != itend; ++it)
        {
            codepdata &cp = GetTexFromCodepoint(*it);
            unsigned char* tx = cp.tex;
            glm::vec3 trans = v_offs + glm::vec3(
				cp.xofs * scale.x * scale.y / SDF_SIZE,
				cp.yofs * scale.y / SDF_SIZE, 0);
            glm::mat4 dx;

            if (*it == 10) // utf-32 line feed
            {
                line++;
                v_offs.x = position.x;
				v_offs.y = position.y + (line + 1) * scale.y;
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

            shader->apply_draw_state({projection, &dx, false, color_});

            renderer::do_quad_draw();

            advance:
            utf8::iterator<const char*> next = it;
            next++;
            if (next != itend)
            {
                float aW = stbtt_GetCodepointKernAdvance(info.get(), *it, *next);
                int bW;
                stbtt_GetCodepointHMetrics(info.get(), *it, &bW, nullptr);
                v_offs.x += (aW * realscale + bW * realscale) * scale.x  * scale.y / SDF_SIZE;
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

        lnvbo->assign(xx);

        NeedsUpdate = false;
    }
}

void Line::emit_draw_calls(DrawCallSink &sink, const uint32_t z) const
{
    sink.submit_line(z, Vec2(x1, y1), Vec2(x2, y2), {R, G, B, A});
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
	set_default_shader_parameters(false);
    Shader::Default::set_color(color_.red, color_.green, color_.blue, color_.alpha);

    Font->bind();

    // Assign position attrib. pointer
	renderer::quad_buffer->bind();
    glVertexAttribPointer(Shader::enable_attrib_array(A_POSITION), 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, nullptr);

	renderer::color_buffer->bind();
    glVertexAttribPointer(Shader::enable_attrib_array(A_COLOR), 4, GL_FLOAT, GL_FALSE, sizeof(float) * 4, nullptr);

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
        Mat4 RenderTransform = Transform * CharPosition[*Text].as_matrix();

        // Assign transformation matrix
        Shader::set_uniform(Shader::Default::uniform(Shader::Default::Uniform::ModelView), &(RenderTransform[0][0]));

        // Assign vertex UVs
        CharPosition[*Text].bind_texture_vbo();
        glVertexAttribPointer(Shader::enable_attrib_array(A_UV), 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, nullptr);

        // Do the rendering!
        renderer::do_quad_draw();

        Shader::disable_attrib_array(A_UV);

        Character += RenderSize.x;
    }

    Shader::disable_attrib_array(A_POSITION);
    Shader::disable_attrib_array(A_COLOR);
}

uint32_t VBO::last_bound_ = 0;
uint32_t VBO::last_bound_index_ = 0;

VBO::VBO(const Type t, const uint32_t elements, const uint32_t size, const IdxKind kind)
{
    internal_vbo_ = 0;
    is_valid_ = false;
    m_type_ = t;
    m_kind_ = kind;
    GameWindow::get_instance().add_vbo(this);

    element_count_ = elements;
    element_size_ = size;
    vbo_data_.reset(new uint8_t[element_size_ * element_count_]);
}

VBO::~VBO()
{
    if (internal_vbo_)
    {
        glDeleteBuffers(1, &internal_vbo_);
        internal_vbo_ = 0;
    }

    GameWindow::get_instance().remove_vbo(this);
}

uint32_t VBO::get_element_count() const
{
    return element_count_;
}

void VBO::invalidate()
{
    is_valid_ = false;
}

void VBO::validate()
{
    if (!is_valid_)
        upload_to_gpu();
}

unsigned int up_type_for_kind(const VBO::Type m_type)
{
    auto UpType = 0;

    if (m_type == VBO::Stream)
        UpType = GL_STREAM_DRAW;
    else if (m_type == VBO::Dynamic)
        UpType = GL_DYNAMIC_DRAW;
    else if (m_type == VBO::Static)
        UpType = GL_STATIC_DRAW;

    return UpType;
}

unsigned int BufTypeForKind(const VBO::IdxKind m_kind)
{
    unsigned int BufType = GL_ARRAY_BUFFER;

    if (m_kind == VBO::ArrayBuffer)
        BufType = GL_ARRAY_BUFFER;
    else if (m_kind == VBO::IndexBuffer)
        BufType = GL_ELEMENT_ARRAY_BUFFER;
    return BufType;
}

void VBO::upload_to_gpu() {
    bool regen_buffer = false;

    if (!is_valid_)
    {
        glGenBuffers(1, &internal_vbo_);
        is_valid_ = true;
        regen_buffer = true;
    }

    bind(true);
    const unsigned int up_type = up_type_for_kind(m_type_);
    const unsigned int buf_type = BufTypeForKind(m_kind_);

    if (regen_buffer)
        glBufferData(buf_type, element_size_ * element_count_, vbo_data_.get(), up_type);
    else
        glBufferSubData(buf_type, 0, element_size_ * element_count_, vbo_data_.get());
}

void VBO::assign(const void* data)
{
    memmove(vbo_data_.get(), data, element_size_ * element_count_);
    upload_to_gpu();
}

void VBO::bind(const bool force) const
{
    assert(is_valid_);

    if (m_kind_ == ArrayBuffer && (last_bound_ != internal_vbo_ || force))
    {
        glBindBuffer(BufTypeForKind(m_kind_), internal_vbo_);
        last_bound_ = internal_vbo_;
    }
    else if (m_kind_ == IndexBuffer && (last_bound_index_ != internal_vbo_ || force))
    {
        glBindBuffer(BufTypeForKind(m_kind_), internal_vbo_);
        last_bound_index_ = internal_vbo_;
    }
}
