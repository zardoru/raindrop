#ifdef WIN32
#include <windows.h>
#endif

#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp>

#include "Global.h"

#include "GameWindow.h"
#include "VBO.h"
#include "Sprite.h"
#include "Transformation.h"

#include "Rendering.h"

#include "Line.h"
#include "Image.h"
#include "ImageLoader.h"

#include "stb_truetype.h"
#include "TruetypeFont.h"
#include "BitmapFont.h"
#include "utf8.h"

#include "Logging.h"

VBO* QuadBuffer = NULL;
VBO* TextureBuffer = NULL;
VBO* ColorBuffer = NULL;

const ColorRGB White = { 1, 1, 1, 1 };
const ColorRGB Black = { 0, 0, 0, 1 };
const ColorRGB Red = { 1, 0, 0, 1 };
const ColorRGB Green = { 0, 1, 0, 1 };
const ColorRGB Blue = { 0, 0, 1, 1 };

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


void SetBlendingMode(RBlendMode Mode)
{
	if (Mode == MODE_ADD)
	{
		glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	}
	else if (Mode == MODE_ALPHA)
	{
		glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
}

void SetPrimitiveQuadVBO()
{
	QuadBuffer->Bind();
	glVertexAttribPointer(WindowFrame.EnableAttribArray(A_POSITION), 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);
	glVertexAttribPointer(WindowFrame.EnableAttribArray(A_UV), 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);
	ColorBuffer->Bind();
	glVertexAttribPointer(WindowFrame.EnableAttribArray(A_COLOR), 4, GL_FLOAT, GL_FALSE, sizeof(float) * 4, 0);
}

void SetTexturedQuadVBO(VBO *TexQuad)
{
	QuadBuffer->Bind();
	glVertexAttribPointer(WindowFrame.EnableAttribArray(A_POSITION), 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);
	TexQuad->Bind();
	glVertexAttribPointer(WindowFrame.EnableAttribArray(A_UV), 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);
	ColorBuffer->Bind();
	glVertexAttribPointer(WindowFrame.EnableAttribArray(A_COLOR), 4, GL_FLOAT, GL_FALSE, sizeof(float) * 4, 0);
}

void FinalizeDraw()
{
	WindowFrame.DisableAttribArray(A_POSITION);
	WindowFrame.DisableAttribArray(A_UV);
	WindowFrame.DisableAttribArray(A_COLOR);
}

void SetShaderParameters(bool InvertColor,
	bool UseGlobalLight, bool Centered, bool UseSecondTransformationMatrix,
	bool BlackToTransparent, bool ReplaceColor,
	int8 HiddenMode)
{
	WindowFrame.SetUniform(U_INVERT, InvertColor);
	WindowFrame.SetUniform(U_LIGHT, UseGlobalLight);

	if (HiddenMode == -1)
		WindowFrame.SetUniform(U_HIDDEN, 0); // not affected by hidden lightning
	else
		WindowFrame.SetUniform(U_HIDDEN, HiddenMode); // Assume the other related parameters are already set.

	WindowFrame.SetUniform(U_REPCOLOR, ReplaceColor);
	WindowFrame.SetUniform(U_BTRANSP, BlackToTransparent);

	WindowFrame.SetUniform(U_TRANSL, UseSecondTransformationMatrix);
	WindowFrame.SetUniform(U_CENTERED, Centered);
}

void DrawTexturedQuad(Image* ToDraw, const AABB& TextureCrop, const Transformation& QuadTransformation, 
	const RBlendMode &Mode, const ColorRGB &InColor)
{
	if (ToDraw)
		ToDraw->Bind();
	else return;

	WindowFrame.SetUniform(U_COLOR, InColor.Red, InColor.Green, InColor.Blue, InColor.Alpha);

	SetBlendingMode(Mode);


	float CropPositions[8] = {
		// topright
		TextureCrop.P2.X / (float)ToDraw->w,
		TextureCrop.P1.Y / (float)ToDraw->h,
		// bottom right
		TextureCrop.P2.X / (float)ToDraw->w,
		TextureCrop.P2.Y / (float)ToDraw->h,
		// bottom left
		TextureCrop.P1.X / (float)ToDraw->w,
		TextureCrop.P2.Y / (float)ToDraw->h,
		// topleft
		TextureCrop.P1.X / (float)ToDraw->w,
		TextureCrop.P1.Y / (float)ToDraw->h,
	};

	TextureBuffer->AssignData(CropPositions);
	SetTexturedQuadVBO(TextureBuffer);

	DoQuadDraw();

	FinalizeDraw();
}

void DoQuadDraw()
{
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void DrawPrimitiveQuad(Transformation &QuadTransformation, const RBlendMode &Mode, const ColorRGB &Color)
{
	Image::BindNull();
	WindowFrame.SetUniform(U_COLOR, Color.Red, Color.Green, Color.Blue, Color.Alpha);

	SetBlendingMode(Mode);

	Mat4 Mat = QuadTransformation.GetMatrix();
	WindowFrame.SetUniform(U_MVP, &(Mat[0][0]));

	// Assign position attrib. pointer
	SetPrimitiveQuadVBO();
	DoQuadDraw();
	FinalizeDraw();

	Image::ForceRebind();
}

void InitializeRender()
{
	static bool Initialized = false;
	if (!Initialized)
	{
		QuadBuffer = new VBO(VBO::Static, sizeof(QuadPositions) / sizeof(float));
		QuadBuffer->Validate();
		QuadBuffer->AssignData(QuadPositions);

		TextureBuffer = new VBO(VBO::Static, sizeof(QuadPositions) / sizeof(float));
		TextureBuffer->Validate();
		TextureBuffer->AssignData(QuadPositions);

		ColorBuffer = new VBO(VBO::Static, sizeof(QuadColours) / sizeof(float));
		ColorBuffer->Validate();
		ColorBuffer->AssignData(QuadColours);
		Initialized = true;
	}
}

void Sprite::UpdateTexture()
{
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

void Sprite::Render()
{
	if (mImage)
	{
		mImage->Bind();
	}else
		return;

	UpdateTexture();

	if (Alpha == 0) return;

	SetBlendingMode(BlendingMode);

	// Assign our matrix.
	SetShaderParameters(ColorInvert, AffectedByLightning, Centered, false, BlackToTransparent);
	
	Mat4 Mat = GetMatrix();
	WindowFrame.SetUniform(U_MVP,  &(Mat[0][0]));

	// Set the color.
	WindowFrame.SetUniform(U_COLOR, Red, Green, Blue, Alpha);

	SetTexturedQuadVBO(UvBuffer);
	DoQuadDraw();

	if (Lighten)
	{
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		WindowFrame.SetUniform(U_COLOR, Red, Green, Blue, Alpha * LightenFactor);
		DoQuadDraw();
	}

	FinalizeDraw();
}

void Sprite::Cleanup()
{
	if (DoTextureCleanup)
	{
		delete UvBuffer;
	}
}

void TruetypeFont::SetupTexture()
{
	Texform = new VBO (VBO::Dynamic, 8);
	Texform->Validate();
	Texform->AssignData(QuadPositions);
}

void TruetypeFont::ReleaseCodepoint(int cp)
{
	if (Texes.find(cp) != Texes.end())
	{
		free(Texes[cp].tex);
		glDeleteTextures(1, &Texes[cp].gltx);
		Texes.erase(cp);
	}
}

void TruetypeFont::Render(const GString &In, const Vec2 &Position, const Mat4 &Transform)
{
	const char* Text = In.c_str(); 
	int Line = 0;
	glm::vec3 vOffs (Position.x, Position.y + scale, 16);

	if (!IsValid)
		return;

	UpdateWindowScale();

	SetBlendingMode(MODE_ALPHA);

	SetShaderParameters(false, false, false, false, false, true);
	WindowFrame.SetUniform(U_COLOR, Red, Green, Blue, Alpha);
	SetPrimitiveQuadVBO();

	try {
		utf8::iterator<const char*> it(Text, Text, Text + In.length());
		utf8::iterator<const char*> itend(Text + In.length(), Text, Text + In.length());
		for (; it != itend; ++it)
		{
			CheckCodepoint(*it); // Force a regeneration of this if necessary
			codepdata &cp = GetTexFromCodepoint(*it);
			unsigned char* tx = cp.tex;
			glm::vec3 trans = vOffs + glm::vec3(cp.xofs, cp.yofs, 0);
			glm::mat4 dx;

			if (*it == 10) // utf-32 line feed
			{
				Line++;
				vOffs.x = Position.x;
				vOffs.y = Position.y + scale * (Line + 1);
				continue;
			}

			dx = Transform * glm::translate(Mat4(), trans) * glm::scale(Mat4(), glm::vec3(cp.w, cp.h, 1));

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

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

				glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, cp.tw, cp.th, 0, GL_ALPHA, GL_UNSIGNED_BYTE, tx);
			}
			else
				glBindTexture(GL_TEXTURE_2D, cp.gltx);

			WindowFrame.SetUniform(U_MVP, &(dx[0][0]));

			DoQuadDraw();

			utf8::iterator<const char*> next = it;
			next++;
			if (next != itend)
			{
				float aW = stbtt_GetCodepointKernAdvance(info, *it, *next);
				int bW;
				stbtt_GetCodepointHMetrics(info, *it, &bW, NULL);
				vOffs.x += aW * virtualscale + bW * virtualscale;
			}
		}
	}
#ifndef NDEBUG
	catch (utf8::exception &ex)
	{

		Utility::DebugBreak();
		Log::Logf("Invalid UTF-8 string %s was passed. Error type: %s\n", ex.what());

	}
#else
	catch (...)
	{
		// nothing
	}
#endif

	FinalizeDraw();
	Image::ForceRebind();

}

void TruetypeFont::ReleaseTextures()
{
	for (std::map <int, codepdata>::iterator i = Texes.begin();
		i != Texes.end();
		i++)
	{
		free (i->second.tex);
		glDeleteTextures(1, &i->second.gltx);
	}

	delete Texform;
}

void Line::UpdateVBO()
{
	if (NeedsUpdate)
	{
		if (!lnvbo)
		{
			lnvbo = new VBO(VBO::Dynamic, 4);
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
	Mat4 Identity;
	UpdateVBO();

	glDisable(GL_DEPTH_TEST);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Set the color.
	SetShaderParameters(false, false, false, false, false, true);

	WindowFrame.SetUniform(U_COLOR, R, G, B, A);
	WindowFrame.SetUniform(U_MVP, &(Identity[0][0]));

	// Assign position attrib. pointer
	lnvbo->Bind();
	glVertexAttribPointer( WindowFrame.EnableAttribArray(A_POSITION), 2, GL_FLOAT, GL_FALSE, 0, 0 );

	ColorBuffer->Bind();
	glVertexAttribPointer(WindowFrame.EnableAttribArray(A_COLOR), 4, GL_FLOAT, GL_FALSE, sizeof(float) * 4, 0);

	glDrawArrays(GL_LINES, 0, 2);

	WindowFrame.DisableAttribArray(A_POSITION);
	WindowFrame.DisableAttribArray(A_COLOR);

	Image::ForceRebind();

	glEnable(GL_DEPTH_TEST);
}

void BitmapFont::Render(const GString &In, const Vec2 &Position, const Mat4 &Transform)
{
	const char* Text = In.c_str();
	int32 Character = 0, Line = 0;
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

	SetShaderParameters(false, false, false);
	WindowFrame.SetUniform(U_COLOR, Red, Green, Blue, Alpha);
	
	Font->Bind();
	
	// Assign position attrib. pointer
	QuadBuffer->Bind();
	glVertexAttribPointer( WindowFrame.EnableAttribArray(A_POSITION), 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0 );

	ColorBuffer->Bind();
	glVertexAttribPointer(WindowFrame.EnableAttribArray(A_COLOR), 4, GL_FLOAT, GL_FALSE, sizeof(float) * 4, 0);

	for (;*Text != '\0'; Text++)
	{
		if(*Text == '\n')
		{
			Character = 0;
			Line += RenderSize.y;
			continue;
		}

		if (*Text < 0)
			continue;

		CharPosition[*Text].SetPosition(Position.x+Character, Position.y+Line);
		Mat4 RenderTransform = Transform * CharPosition[*Text].GetMatrix();

		// Assign transformation matrix
		WindowFrame.SetUniform(U_MVP,  &(RenderTransform[0][0]));

		// Assign vertex UVs
		CharPosition[*Text].BindTextureVBO();
		glVertexAttribPointer( WindowFrame.EnableAttribArray(A_UV), 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0 );

		// Do the rendering!
		DoQuadDraw();
		
		WindowFrame.DisableAttribArray(A_UV);

		Character += RenderSize.x;
	}

	WindowFrame.DisableAttribArray(A_POSITION);
	WindowFrame.DisableAttribArray(A_COLOR);
}

uint32 VBO::LastBound = 0;
uint32 VBO::LastBoundIndex = 0;

VBO::VBO(Type T, uint32 Elements, uint32 Size, IdxKind Kind)
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

	delete VboData;
	VboData = NULL;
}

uint32 VBO::GetElementCount()
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
	{
		glGenBuffers(1, &InternalVBO);
		AssignData(VboData);
		IsValid = true;
	}
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

	UpType = UpTypeForKind(mType);
	BufType = BufTypeForKind(mKind);

	memmove(VboData, Data, ElementSize * ElementCount);

	if (!IsValid)
	{
		glGenBuffers(1, &InternalVBO);
		IsValid = true;
	}

	Bind();
	glBufferData(BufType, ElementSize * ElementCount, VboData, UpType);
}


void VBO::Bind()
{
	assert(IsValid);

	if (mKind == ArrayBuffer && LastBound != InternalVBO)
	{
		glBindBuffer(BufTypeForKind(mKind), InternalVBO);
		LastBound = InternalVBO;
	}
	else if (mKind == IndexBuffer && LastBoundIndex != InternalVBO)
	{
		glBindBuffer(BufTypeForKind(mKind), InternalVBO);
		LastBoundIndex = InternalVBO;
	}
}
