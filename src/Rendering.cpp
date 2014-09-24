#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp>

#include "Global.h"

#include "GameWindow.h"
#include "Rendering.h"
#include "VBO.h"
#include "GraphObject2D.h"
#include "Image.h"
#include "ImageLoader.h"

#include "stb_truetype.h"
#include "TruetypeFont.h"
#include "utf8.h"

VBO* GraphObject2D::mBuffer;

float QuadPositions[8] =
	{
		1,
		0,

		1,
		1,

		0,
		1,

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

	WindowFrame.SetUniform(U_TRANSL, false);
	WindowFrame.SetUniform(U_CENTERED, Centered);

	// Assign position attrib. pointer
	mBuffer->Bind();

	glVertexAttribPointer( WindowFrame.EnableAttribArray(A_POSITION), 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0 );

	// assign vertex UVs
	UvBuffer->Bind();
	glVertexAttribPointer( WindowFrame.EnableAttribArray(A_UV), 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0 );

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

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
	Texform->AssignData(QuadPositions);

	glGenTextures(1, &tex);
}

void TruetypeFont::Render (const char* Text, const Vec2 &Position, const Vec2 &Scale)
{
	std::vector<int> r; 
	glm::vec3 vOffs (Position.x, Position.y, 16);
	utf8::utf8to32(Text, Text + Utility::Widen(Text).length(), std::back_inserter(r));

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0);

	glBindTexture(GL_TEXTURE_2D, tex);

	// Set the color.
	WindowFrame.SetUniform(U_COLOR, 1, 1, 1, 1);
	WindowFrame.SetUniform(U_INVERT, false);
	WindowFrame.SetUniform(U_LIGHT, false);
	WindowFrame.SetUniform(U_HIDDEN, 0); // not affected by hidden lightning

	WindowFrame.SetUniform(U_TRANSL, false);
	WindowFrame.SetUniform(U_CENTERED, false);

	// Assign position attrib. pointer
	GraphObject2D::BindTopLeftVBO();
	glVertexAttribPointer( WindowFrame.EnableAttribArray(A_POSITION), 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0 );

	// assign vertex UVs
	GraphObject2D::BindTopLeftVBO();
	glVertexAttribPointer( WindowFrame.EnableAttribArray(A_UV), 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0 );

	size_t s = strlen(Text);
	for (int i = 0; i < s; i++)
	{
		int w, h, xofs, yofs;
		unsigned char* tx = stbtt_GetCodepointBitmap (info, 0, stbtt_ScaleForPixelHeight(info, Scale.y), r[i], &w, &h, &xofs, &yofs);
		glm::mat4 dx;
		dx = glm::translate(Mat4(), vOffs) * glm::scale(Mat4(), glm::vec3(w, h, 1));
		
		// do the actual draw?
		glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, w, h, 0, GL_ALPHA, GL_UNSIGNED_BYTE, tx);

		WindowFrame.SetUniform(U_MVP,  &(dx[0][0]));

		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

		stbtt_FreeBitmap(tx, NULL);

		if (i+1 < s)
			vOffs.x += w;
	}

	WindowFrame.DisableAttribArray(A_POSITION);
	WindowFrame.DisableAttribArray(A_UV);
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