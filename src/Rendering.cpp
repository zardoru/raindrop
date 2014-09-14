#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp>

#include "Global.h"

#include "GameWindow.h"
#include "Rendering.h"
#include "VBO.h"
#include "GraphObject2D.h"
#include "Image.h"
#include "ImageLoader.h"

VBO* GraphObject2D::mBuffer;

void GraphObject2D::InitVBO()
{
	float Positions[8] =
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

	// since shaders are already loaded from graphman's init functions..
	// we'll deal with what we need to deal.
	// Our initial quad.

	mBuffer->Validate();
	mBuffer->AssignData(Positions);
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