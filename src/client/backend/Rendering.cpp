#include <filesystem>
#include <map>
#include <rmath.h>
#include <glm.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <utf8.h>
#include <stb/stb_truetype.h>
#include <TextAndFileUtil.h>

#include "GameWindow.h"

#include "VBO.h"
#include "Transformation.h"
#include "Rendering.h"
#include "Sprite.h"


#include "Line.h"
#include "Texture.h"

#include "TruetypeFont.h"
#include "BitmapFont.h"

#include "Shader.h"

#include "ImageLoader.h"

#include "../structure/Configuration.h"

const ColorRGB White = { 1, 1, 1, 1 };
const ColorRGB Black = { 0, 0, 0, 1 };
const ColorRGB Red = { 1, 0, 0, 1 };
const ColorRGB Green = { 0, 1, 0, 1 };
const ColorRGB Blue = { 0, 0, 1, 1 };


namespace Renderer {
	VBO* QuadBuffer = nullptr;
	VBO* TextureBuffer = nullptr;
	VBO* TempTextureBuffer = nullptr;
	VBO* ColorBuffer = nullptr;
	Texture* xor_tex = nullptr;

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

	void DoQuadDraw()
	{
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	}

	void DrawPrimitiveQuad(Transformation &QuadTransformation, const EBlendMode &Mode, const ColorRGB &Color)
	{
		Texture::Unbind();
		Shader::SetUniform(DefaultShader::GetUniform(U_COLOR), Color.Red, Color.Green, Color.Blue, Color.Alpha);

		SetBlendingMode(Mode);

		Mat4 Mat = QuadTransformation.GetMatrix();
		Shader::SetUniform(DefaultShader::GetUniform(U_MVP), &(Mat[0][0]));

		// Assign position attrib. pointer
		SetPrimitiveQuadVBO();
		DoQuadDraw();
		FinalizeDraw();

		Texture::ForceRebind();
	}

	void SetXorTexParameters() {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}

	static bool Initialized = false;
	void InitializeRender()
	{
		if (!Initialized)
		{
			QuadBuffer = new VBO(VBO::Static, sizeof(QuadPositions) / sizeof(float));
			QuadBuffer->AssignData(QuadPositions);
			assert(glGetError() == 0);

			TextureBuffer = new VBO(VBO::Static, sizeof(QuadPositions) / sizeof(float));
			TextureBuffer->AssignData(QuadPositions);
            assert(glGetError() == 0);

			TempTextureBuffer = new VBO(VBO::Static, sizeof(QuadPositions) / sizeof(float));
			TempTextureBuffer->AssignData(QuadPositions);
            assert(glGetError() == 0);

			ColorBuffer = new VBO(VBO::Static, sizeof(QuadColours) / sizeof(float));
			ColorBuffer->AssignData(QuadColours);
            assert(glGetError() == 0);

			// create xor texture
			xor_tex = new Texture;
			
			std::vector<uint32_t> buf(256 * 256);
			for(int i = 0; i < 256; i++) {
				for (int j = 0; j < 256; j++) {
					uint8_t x = i ^ j;

					buf[i * 256 + j] = (255 << 24) + x + (x << 8) + (x << 16);
				}
			}

			ImageData d(256, 256, nullptr);
			d.Data = buf;

			xor_tex->SetTextureData2D(d);
            assert(glGetError() == 0);

			xor_tex->fname = "xor";

			SetXorTexParameters(); // it's bound by SetTextureData2D, apply parameters
            assert(glGetError() == 0);

			ImageLoader::RegisterTexture(xor_tex);
            assert(glGetError() == 0);


            Initialized = true;
		}
	}

	Texture* GetXorTexture(){
		return xor_tex;
	}

	

	void SetTextureParameters(std::string Dir)
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

	void SetBlendingMode(EBlendMode Mode)
	{
		if (Mode == BLEND_ADD)
		{
			glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		}
		else if (Mode == BLEND_ALPHA)
		{
			glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}
	}

	void SetPrimitiveQuadVBO()
	{
		QuadBuffer->Bind();
		glVertexAttribPointer(Shader::EnableAttribArray(DefaultShader::GetUniform(A_POSITION)), 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, nullptr);
		glVertexAttribPointer(Shader::EnableAttribArray(DefaultShader::GetUniform(A_UV)), 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, nullptr);
		ColorBuffer->Bind();
		glVertexAttribPointer(Shader::EnableAttribArray(DefaultShader::GetUniform(A_COLOR)), 4, GL_FLOAT, GL_FALSE, sizeof(float) * 4, nullptr);
	}

	void SetTexturedQuadVBO(VBO *TexQuad)
	{
		QuadBuffer->Bind();
		glVertexAttribPointer(Shader::EnableAttribArray(DefaultShader::GetUniform(A_POSITION)), 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, nullptr);
		TexQuad->Bind();
		glVertexAttribPointer(Shader::EnableAttribArray(DefaultShader::GetUniform(A_UV)), 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, nullptr);
		ColorBuffer->Bind();
		glVertexAttribPointer(Shader::EnableAttribArray(DefaultShader::GetUniform(A_COLOR)), 4, GL_FLOAT, GL_FALSE, sizeof(float) * 4, nullptr);
	}

	void FinalizeDraw()
	{
		Shader::DisableAttribArray(DefaultShader::GetUniform(A_POSITION));
		Shader::DisableAttribArray(DefaultShader::GetUniform(A_UV));
		Shader::DisableAttribArray(DefaultShader::GetUniform(A_COLOR));
	}

	void SetShaderParameters(bool InvertColor,
		bool UseGlobalLight, bool Centered, bool UseSecondTransformationMatrix,
		bool BlackToTransparent, bool ReplaceColor,
		int8_t HiddenMode)
	{
		DefaultShader::StaticBind();
		Shader::SetUniform(DefaultShader::GetUniform(U_INVERT), InvertColor);

		if (HiddenMode == -1)
			Shader::SetUniform(DefaultShader::GetUniform(U_HIDDEN), 0); // not affected by hidden lightning
		else
			Shader::SetUniform(DefaultShader::GetUniform(U_HIDDEN), HiddenMode); // Assume the other related parameters are already set.

		Shader::SetUniform(DefaultShader::GetUniform(U_REPCOLOR), ReplaceColor);
		Shader::SetUniform(DefaultShader::GetUniform(U_BTRANSP), BlackToTransparent);

		Shader::SetUniform(DefaultShader::GetUniform(U_CENTERED), Centered);
	}

	void DrawTexturedQuad(Texture* ToDraw, const AABB& TextureCrop, const Transformation& QuadTransformation,
		const EBlendMode &Mode, const ColorRGB &InColor)
	{
		if (ToDraw)
			ToDraw->Bind();
		else return;

		Shader::SetUniform(DefaultShader::GetUniform(U_COLOR), InColor.Red, InColor.Green, InColor.Blue, InColor.Alpha);

		SetBlendingMode(Mode);

		float CropPositions[8] = {
			// topright
			TextureCrop.P2.X / float(ToDraw->w),
			TextureCrop.P2.Y / float(ToDraw->h),
			// bottom right
			TextureCrop.P2.X / float(ToDraw->w),
			TextureCrop.P1.Y / float(ToDraw->h),
			// bottom left
			TextureCrop.P1.X / float(ToDraw->w),
			TextureCrop.P1.Y / float(ToDraw->h),
			// topleft
			TextureCrop.P1.X / float(ToDraw->w),
			TextureCrop.P2.Y / float(ToDraw->h),
		};

		TempTextureBuffer->AssignData(CropPositions);
		SetTexturedQuadVBO(TempTextureBuffer);

		DoQuadDraw();

		FinalizeDraw();
	}
	
	VBO* GetDefaultGeometryBuffer()
	{
		return QuadBuffer;
	}
	
	VBO* GetDefaultTextureBuffer()
	{
		return TextureBuffer;
	}

	VBO* GetDefaultColorBuffer()
	{
		return ColorBuffer;
	}

	void SetCurrentObjectMatrix(glm::mat4 &mat)
	{
	    Shader::SetUniform(DefaultShader::GetUniform(U_MVP), &(mat[0][0]));
	}

	void SetScissor(bool enabled)
	{
		if (enabled) glEnable(GL_SCISSOR_TEST);
		else glDisable(GL_SCISSOR_TEST);
	}

	void SetScissorRegion(int x, int y, int w, int h) {
		float ratio = WindowFrame.GetWindowVScale();
		glScissor(x * ratio, (ScreenHeight - (y + h)) * ratio, w * ratio, h * ratio);
	}

	void SetScissorRegionWnd(int x, int y, int w, int h) {
		// x: 0 -> 0; screenwidth -> windowwidth
		// y: 0 -> windowheight; screenheight -> 0
		auto tx = [&](int x) {
			auto ww = WindowFrame.GetWindowSize().x;
			return x * ww / ScreenWidth;
		};

		auto ty = [&](int y) {
			auto wh = WindowFrame.GetWindowSize().y;
			auto m = -wh / ScreenHeight;
			return m * y + wh;
		};

		auto vratio = WindowFrame.GetWindowSize().y / ScreenHeight;
		glScissor(tx(x), ty(y) - h * vratio, tx(w), h * vratio);
	}
}

void Sprite::UpdateTexture()
{
	if (!Renderer::Initialized) return;

    if (!DirtyTexture)
        return;

    if (!DoTextureCleanup) // We must not own a UV buffer.
    {
        DirtyTexture = false;
        return;
    }

    if (!UvBuffer)
        UvBuffer = new VBO(VBO::Dynamic, 8);

    UvBuffer->Validate();

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

    UvBuffer->AssignData(CropPositions);
    DirtyTexture = false;
}

bool Sprite::ShouldDraw()
{
    if (Alpha == 0)
        return false;

	if (mShader)
		if (!mShader->IsValid())
			return false;

    if (mTexture)
    {
		mTexture->Bind();
		return mTexture->IsBound();
    }
    else
        return mShader && mShader->IsValid();

    return true;
}

bool Sprite::RenderMinimalSetup()
{
    if (!ShouldDraw())
        return false;

    UpdateTexture();

    Renderer::SetBlendingMode(BlendingMode);

    UvBuffer->Bind();
    glVertexAttribPointer(
		Renderer::Shader::EnableAttribArray(Renderer::DefaultShader::GetUniform(Renderer::A_UV)), 
		2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, nullptr
	);

    // Set the color.
	auto lf = 1.0 + LightenFactor;
	if (!Lighten)
		Renderer::DefaultShader::SetColor(Color.Red, Color.Green, Color.Blue, Alpha);
	else
		Renderer::DefaultShader::SetColor(Color.Red * lf, Color.Green * lf, Color.Blue * lf, Alpha);

    Renderer::DoQuadDraw();

    return true;
}

void Sprite::Render()
{
    if (!ShouldDraw())
        return;

    UpdateTexture();
    assert(glGetError() == 0);

    Renderer::SetBlendingMode(BlendingMode);
    assert(glGetError() == 0);

    // Assign our matrix.
	if (!mShader) {
		auto mat = GetMatrix();
		Renderer::SetShaderParameters(ColorInvert, AffectedByLightning, Centered, false, BlackToTransparent);
        assert(glGetError() == 0);

		auto lf = 1.0 + LightenFactor;
		if (!Lighten)
			Renderer::DefaultShader::SetColor(Color.Red, Color.Green, Color.Blue, Alpha);
		else
			Renderer::DefaultShader::SetColor(Color.Red * lf, Color.Green * lf, Color.Blue * lf, Alpha);
        assert(glGetError() == 0);
		
		Renderer::SetCurrentObjectMatrix(mat);
        assert(glGetError() == 0);
	}
	else {
		auto proj = WindowFrame.GetMatrixProjection();
		auto mat = GetMatrix();

		mShader->Bind();

		auto sh = mShader->GetUniform("projection"); assert(glGetError() == 0);
		if (sh != -1)
			Renderer::Shader::SetUniform(sh, &proj[0][0]);
        assert(glGetError() == 0);

		sh = mShader->GetUniform("mvp"); assert(glGetError() == 0);
		if (sh != -1)
			Renderer::Shader::SetUniform(sh, &mat[0][0]); assert(glGetError() == 0);

		sh = mShader->GetUniform("centered"); assert(glGetError() == 0);
		if (sh != -1)
			Renderer::Shader::SetUniform(sh, Centered); assert(glGetError() == 0);

		sh = mShader->GetUniform("color"); assert(glGetError() == 0);
		if (sh != -1)
			Renderer::Shader::SetUniform(sh, 
				l2gamma(Color.Red), 
				l2gamma(Color.Green), 
				l2gamma(Color.Blue), 
				Alpha);
	    }

    assert(glGetError() == 0);

    

    Renderer::SetTexturedQuadVBO(UvBuffer);
    assert(glGetError() == 0);

    Renderer::DoQuadDraw();
    assert(glGetError() == 0);

    Renderer::FinalizeDraw();
    assert(glGetError() == 0);
}

void Sprite::Cleanup()
{
    if (DoTextureCleanup)
    {
        delete UvBuffer;
		UvBuffer = nullptr;
    }
}

void TruetypeFont::ReleaseCodepoint(int cp)
{
    if (Texes->find(cp) != Texes->end())
    {
        free(Texes->at(cp).tex);
        glDeleteTextures(1, &Texes->at(cp).gltx);
        Texes->erase(cp);
    }
}

void TruetypeFont::Render(const std::string &In, const Vec2 &Position, const Mat4 &Transform, const Vec2 &Scale)
{
    const char* Text = In.c_str();
    int Line = 0;
	size_t len = In.length();
    glm::vec3 vOffs(Position.x, Position.y + Scale.y, 0);

    if (!IsValid)
        return;

    Renderer::DefaultShader::StaticBind();
    Renderer::SetBlendingMode(BLEND_ALPHA);
    Renderer::SetShaderParameters(false, false, false, false, false, true);
    Renderer::DefaultShader::SetColor(Red, Green, Blue, Alpha);
    Renderer::SetPrimitiveQuadVBO();

    try
    {
		auto nd = utf8::find_invalid<const char*>(Text, Text + In.length());
        utf8::iterator<const char*> it(Text, Text, nd);
        utf8::iterator<const char*> itend(nd, Text, nd);
        for (; it != itend; ++it)
        {
            codepdata &cp = GetTexFromCodepoint(*it);
            unsigned char* tx = cp.tex;
            glm::vec3 trans = vOffs + glm::vec3(
				cp.xofs * Scale.x * Scale.y / SDF_SIZE, 
				cp.yofs * Scale.y / SDF_SIZE, 0);
            glm::mat4 dx;

            if (*it == 10) // utf-32 line feed
            {
                Line++;
                vOffs.x = Position.x;
				vOffs.y = Position.y + (Line + 1) * Scale.y;
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

            dx = Transform * glm::translate(glm::identity<Mat4>(), trans) *
                 glm::scale(glm::identity<Mat4>(), glm::vec3(cp.w * Scale.y / SDF_SIZE, cp.h * Scale.y / SDF_SIZE, 1));

            Renderer::Shader::SetUniform(Renderer::DefaultShader::GetUniform(Renderer::U_MVP), &(dx[0][0]));

            Renderer::DoQuadDraw();

            advance:
            utf8::iterator<const char*> next = it;
            next++;
            if (next != itend)
            {
                float aW = stbtt_GetCodepointKernAdvance(info.get(), *it, *next);
                int bW;
                stbtt_GetCodepointHMetrics(info.get(), *it, &bW, NULL);
                vOffs.x += (aW * realscale + bW * realscale) * Scale.x  * Scale.y / SDF_SIZE;
            }
        }
    }
#ifndef NDEBUG
    catch (utf8::exception &ex)
    {
        Utility::DebugBreak();
        //Log::Logf("Invalid UTF-8 string %s was passed. Error type: %s\n", ex.what());
    }
#else
    catch (...)
    {
        // nothing
    }
#endif

    Renderer::FinalizeDraw();
    Texture::ForceRebind();
}

void TruetypeFont::ReleaseTextures()
{
    for (auto i = Texes->begin();
    i != Texes->end();
        ++i)
    {
		if (i->second.tex) {
			free(i->second.tex);
			i->second.tex = 0;
		}
		if (i->second.gltx) {
			glDeleteTextures(1, &i->second.gltx);
			i->second.gltx = 0;
		}
    }
}

void Line::UpdateVBO()
{
    if (NeedsUpdate)
    {
        if (!lnvbo)
        {
            lnvbo = new VBO(VBO::Stream, 4);
        }

        lnvbo->Validate();

        float xx[] = {
            x1, y1,
            x2, y2
        };

        lnvbo->AssignData(xx);

        NeedsUpdate = false;
    }
}

void Line::Render()
{
    auto Identity = glm::identity<Mat4>();
    UpdateVBO();

    glDisable(GL_DEPTH_TEST);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Set the color.
	using namespace Renderer;
	SetShaderParameters(true, false, false, false, false, false);

    DefaultShader::SetColor(R, G, B, A);
    Shader::SetUniform(DefaultShader::GetUniform(U_MVP), &(Identity[0][0]));

    // Assign position attrib. pointer
    lnvbo->Bind();
    glVertexAttribPointer(Shader::EnableAttribArray(DefaultShader::GetUniform(A_POSITION)), 2, GL_FLOAT, GL_FALSE, 0, nullptr);

	ColorBuffer->Bind();
    glVertexAttribPointer(Shader::EnableAttribArray(DefaultShader::GetUniform(A_COLOR)), 4, GL_FLOAT, GL_FALSE, sizeof(float) * 4, nullptr);

    glDrawArrays(GL_LINES, 0, 2);

    Shader::DisableAttribArray(DefaultShader::GetUniform(A_POSITION));
    Shader::DisableAttribArray(DefaultShader::GetUniform(A_COLOR));

    Texture::ForceRebind();

    glEnable(GL_DEPTH_TEST);
}

void BitmapFont::Render(const std::string &In, const Vec2 &Position, const Mat4 &Transform, const Vec2 &Scale)
{
    const char* Text = In.c_str();
    int32_t Character = 0, Line = 0;
    /* OpenGL Code Ahead */

    if (!Font)
        return;

    if (!Font->IsValid)
    {
        for (int i = 0; i < 256; i++)
        {
            CharPosition[i].Invalidate();
            CharPosition[i].Initialize(true);
        }
        Font->IsValid = true;
    }

	using namespace Renderer;
	SetShaderParameters(false, false, false);
    DefaultShader::SetColor(Red, Green, Blue, Alpha);

    Font->Bind();

    // Assign position attrib. pointer
	Renderer::QuadBuffer->Bind();
    glVertexAttribPointer(Shader::EnableAttribArray(DefaultShader::GetUniform(A_POSITION)), 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, nullptr);

	Renderer::ColorBuffer->Bind();
    glVertexAttribPointer(Shader::EnableAttribArray(DefaultShader::GetUniform(A_COLOR)), 4, GL_FLOAT, GL_FALSE, sizeof(float) * 4, nullptr);

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

        CharPosition[*Text].SetPosition(Position.x + Character, Position.y + Line);
        Mat4 RenderTransform = Transform * CharPosition[*Text].GetMatrix();

        // Assign transformation matrix
        Shader::SetUniform(DefaultShader::GetUniform(U_MVP), &(RenderTransform[0][0]));

        // Assign vertex UVs
        CharPosition[*Text].BindTextureVBO();
        glVertexAttribPointer(Shader::EnableAttribArray(DefaultShader::GetUniform(A_UV)), 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, nullptr);

        // Do the rendering!
        Renderer::DoQuadDraw();

        Shader::DisableAttribArray(DefaultShader::GetUniform(A_UV));

        Character += RenderSize.x;
    }

    Shader::DisableAttribArray(DefaultShader::GetUniform(A_POSITION));
    Shader::DisableAttribArray(DefaultShader::GetUniform(A_COLOR));
}

uint32_t VBO::LastBound = 0;
uint32_t VBO::LastBoundIndex = 0;

VBO::VBO(Type T, uint32_t Elements, uint32_t Size, IdxKind Kind)
{
    InternalVBO = 0;
    IsValid = false;
    mType = T;
    mKind = Kind;
    WindowFrame.AddVBO(this);

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

    WindowFrame.RemoveVBO(this);

    delete[] VboData;
    VboData = nullptr;
}

uint32_t VBO::GetElementCount() const
{
    return ElementCount;
}

void VBO::Invalidate()
{
    IsValid = false;
}

void VBO::Validate()
{
    if (!IsValid)
        AssignData(VboData);
}

unsigned int UpTypeForKind(VBO::Type mType)
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

unsigned int BufTypeForKind(VBO::IdxKind mKind)
{
    unsigned int BufType = GL_ARRAY_BUFFER;

    if (mKind == VBO::ArrayBuffer)
        BufType = GL_ARRAY_BUFFER;
    else if (mKind == VBO::IndexBuffer)
        BufType = GL_ELEMENT_ARRAY_BUFFER;
    return BufType;
}

void VBO::AssignData(void* Data)
{
    unsigned int UpType;
    unsigned int BufType;
    bool RegenBuffer = false;

    UpType = UpTypeForKind(mType);
    BufType = BufTypeForKind(mKind);

    memmove(VboData, Data, ElementSize * ElementCount);

    if (!IsValid)
    {
        glGenBuffers(1, &InternalVBO);
        IsValid = true;
        RegenBuffer = true;
    }

    Bind(true);
    if (RegenBuffer)
        glBufferData(BufType, ElementSize * ElementCount, VboData, UpType);
    else
        glBufferSubData(BufType, 0, ElementSize * ElementCount, VboData);
}

void VBO::Bind(bool Force) const
{
    assert(IsValid);

    if (mKind == ArrayBuffer && (LastBound != InternalVBO || Force))
    {
        glBindBuffer(BufTypeForKind(mKind), InternalVBO);
        LastBound = InternalVBO;
    }
    else if (mKind == IndexBuffer && (LastBoundIndex != InternalVBO || Force))
    {
        glBindBuffer(BufTypeForKind(mKind), InternalVBO);
        LastBoundIndex = InternalVBO;
    }
}
