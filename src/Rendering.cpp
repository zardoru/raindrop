#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp>

#include "Global.h"

#include "GameWindow.h"
#include "Rendering.h"
#include "VBO.h"
#include "GraphObject2D.h"
#include "Line.h"
#include "Image.h"
#include "ImageLoader.h"

#include "stb_truetype.h"
#include "TruetypeFont.h"
#include "utf8.h"

VBO* GraphObject2D::mBuffer;

float QuadPositions[8] =
	{
		// tr
		1,
		0,

		// br
		1,
		1,

		// bl
		0,
		1,

		// tl
		0,
		0,
	};

float QuadPositionsTX[8] =
	{
				// tr
		1,
		0,

		// br
		1,
		1,

		// bl
		0,
		1,

		// tl
		0,
		0,
	};

void GraphObject2D::InitVBO()
{
	// since shaders are already loaded from graphman's init functions..
	// we'll deal with what we need to deal.
	// Our initial quad.

	mBuffer->Validate();
	mBuffer->AssignData(QuadPositions);
}

void GraphObject2D::UpdateMatrix()
{
	if (DirtyMatrix)
	{
		Mat4 posMatrix =	glm::scale(
		glm::rotate(
			glm::translate(
					    Mat4(1.0f), 
					    glm::vec3(mPosition.x, mPosition.y, z_order)), 
					mRotation, glm::vec3(0,0,1)
				   ), glm::vec3(mScale.x*mWidth, mScale.y*mHeight, 1));

		Matrix = posMatrix;
		DirtyMatrix = false;
	}
}

void GraphObject2D::UpdateTexture()
{
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

void Image::Bind()
{
	if (LastBound != this)
	{
		glBindTexture(GL_TEXTURE_2D, texture);
		LastBound = this;
	}
}

void GraphObject2D::Render()
{
	
	if (mImage)
	{
		mImage->Bind();
	}else
		return;

	if (DirtyTexture)
		UpdateTexture();

	if (Alpha == 0) return;

	UpdateMatrix();

	if (BlendingMode == MODE_ADD)
	{
		glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	}
	else if (BlendingMode == MODE_ALPHA)
	{
		glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	// Assign our matrix.
	WindowFrame.SetUniform(U_MVP,  &(Matrix[0][0]));

	// Set the color.
	WindowFrame.SetUniform(U_COLOR, Red, Green, Blue, Alpha);
	WindowFrame.SetUniform(U_INVERT, ColorInvert);
	WindowFrame.SetUniform(U_LIGHT, AffectedByLightning);
	WindowFrame.SetUniform(U_HIDDEN, 0); // not affected by hidden lightning
	WindowFrame.SetUniform(U_REPCOLOR, false);

	WindowFrame.SetUniform(U_TRANSL, false);
	WindowFrame.SetUniform(U_CENTERED, Centered);

	// Assign position attrib. pointer
	mBuffer->Bind();

	glVertexAttribPointer( WindowFrame.EnableAttribArray(A_POSITION), 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0 );

	// assign vertex UVs
	UvBuffer->Bind();
	glVertexAttribPointer( WindowFrame.EnableAttribArray(A_UV), 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0 );

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	if (Lighten)
	{
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		WindowFrame.SetUniform(U_COLOR, Red, Green, Blue, Alpha * LightenFactor);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	}

	WindowFrame.DisableAttribArray(A_POSITION);
	WindowFrame.DisableAttribArray(A_UV);
}

void GraphObject2D::Cleanup()
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
	Texform->AssignData(QuadPositionsTX);
}

void TruetypeFont::Render (const char* Text, const Vec2 &Position)
{
	std::vector<int> r; 
	int Line = 0;
	glm::vec3 vOffs (Position.x, Position.y + scale, 16);

	if (!IsValid)
		return;

	utf8::utf8to32(Text, Text + strlen(Text), std::back_inserter(r));

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glBindTexture(GL_TEXTURE_2D, tex);

	// Set the color.
	WindowFrame.SetUniform(U_COLOR, 1, 1, 1, 1);
	WindowFrame.SetUniform(U_INVERT, false);
	WindowFrame.SetUniform(U_LIGHT, false);
	WindowFrame.SetUniform(U_HIDDEN, 0);
	WindowFrame.SetUniform(U_REPCOLOR, true);

	WindowFrame.SetUniform(U_TRANSL, false);
	WindowFrame.SetUniform(U_CENTERED, false);

	// Assign position attrib. pointer
	GraphObject2D::BindTopLeftVBO();
	glVertexAttribPointer( WindowFrame.EnableAttribArray(A_POSITION), 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0 );

	// assign vertex UVs
	GraphObject2D::BindTopLeftVBO();
	glVertexAttribPointer( WindowFrame.EnableAttribArray(A_UV), 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0 );

	size_t s = r.size();
	for (uint32 i = 0; i < s; i++)
	{
		codepdata &cp = GetTexFromCodepoint(r[i]);
		unsigned char* tx = cp.tex;
		glm::vec3 trans = vOffs + glm::vec3(cp.xofs, cp.yofs, 0);
		glm::mat4 dx;

		if (r[i] == 10) // utf-32 line feed
		{
			Line++;
			vOffs.x = Position.x;
			vOffs.y = Position.y + scale * (Line+1);
			continue;
		}

		dx = glm::translate(Mat4(), trans) * glm::scale(Mat4(), glm::vec3(cp.w, cp.h, 1));
		
		// do the actual draw?
		if (cp.gltx == 0)
		{
			glGenTextures(1, &cp.gltx);
			glBindTexture(GL_TEXTURE_2D, cp.gltx);

			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

			glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, cp.w, cp.h, 0, GL_ALPHA, GL_UNSIGNED_BYTE, tx);
		}else
			glBindTexture(GL_TEXTURE_2D, cp.gltx);
		WindowFrame.SetUniform(U_MVP,  &(dx[0][0]));

		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

		if (i+1 < s)
		{
			float aW = stbtt_GetCodepointKernAdvance(info, r[i], r[i+1]);
			int bW;
			stbtt_GetCodepointHMetrics(info, r[i], &bW, NULL);
			vOffs.x += aW * realscale + bW * realscale;
		}
	}

	WindowFrame.DisableAttribArray(A_POSITION);
	WindowFrame.DisableAttribArray(A_UV);
	Image::ForceRebind();
}

void TruetypeFont::ReleaseTextures()
{
	for (std::map <int, codepdata>::iterator i = Texes.begin();
		i != Texes.end();
		i++)
	{
		delete i->second.tex;
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
	WindowFrame.SetUniform(U_COLOR, R, G, B, A);
	WindowFrame.SetUniform(U_INVERT, false);
	WindowFrame.SetUniform(U_LIGHT, false);
	WindowFrame.SetUniform(U_HIDDEN, 0);
	WindowFrame.SetUniform(U_REPCOLOR, true);

	WindowFrame.SetUniform(U_TRANSL, false);
	WindowFrame.SetUniform(U_CENTERED, false);

	WindowFrame.SetUniform(U_MVP, &(Identity[0][0]));

	// Assign position attrib. pointer
	lnvbo->Bind();
	glVertexAttribPointer( WindowFrame.EnableAttribArray(A_POSITION), 2, GL_FLOAT, GL_FALSE, 0, 0 );

	glDrawArrays(GL_LINES, 0, 2);

	WindowFrame.DisableAttribArray(A_POSITION);
	Image::ForceRebind();

	glEnable(GL_DEPTH_TEST);
}

uint32 VBO::LastBound = 0;

VBO::VBO(Type T, uint32 Elements)
{
	InternalVBO = 0;
	IsValid = false;
	mType = T;
	WindowFrame.AddVBO(this);

	ElementCount = Elements;
	VboData = new float[ElementCount];
}

VBO::~VBO()
{
	if (InternalVBO)
		glDeleteBuffers(1, &InternalVBO);

	WindowFrame.RemoveVBO(this);

	delete VboData;
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

void VBO::AssignData(float* Data)
{
	unsigned int UpType;

	if (mType == Stream)
		UpType = GL_STREAM_DRAW;
	else if(mType == Dynamic)
		UpType = GL_DYNAMIC_DRAW;
	else if (mType == Static)
		UpType = GL_STATIC_DRAW;

	memmove(VboData, Data, sizeof(float) * ElementCount);
	Bind();
	// glBufferData(GL_ARRAY_BUFFER, sizeof(float) * ElementCount, NULL, UpType);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * ElementCount, VboData, UpType);
}


void VBO::Bind()
{
	if (LastBound != InternalVBO)
	{
		glBindBuffer(GL_ARRAY_BUFFER, InternalVBO);
		LastBound = InternalVBO;
	}
}