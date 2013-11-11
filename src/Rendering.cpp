#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp>

#include "Global.h"

#include "GameWindow.h"
#include "Rendering.h"
#include "VBO.h"
#include "GraphObject2D.h"
#include "Image.h"
#include "ImageLoader.h"

#ifndef OLD_GL
VBO* GraphObject2D::mBuffer;
VBO* GraphObject2D::mCenteredBuffer;
#endif

void GraphObject2D::InitVBO()
{
#ifndef OLD_GL
	float Positions[12] =
	{
		0,
		0,

		1,
		0,

		1,
		1,

		0,
		0,

		0,
		1,

		1,
		1
	};

	// since shaders are already loaded from graphman's init functions..
	// we'll deal with what we need to deal.
	// Our initial quad.

	mBuffer->Validate();
	mBuffer->AssignData(Positions);

	float PositionsCentered [12] = {
		-0.5,
		-0.5,
		0.5,
		-0.5,
		0.5,
		0.5,
		-0.5,
		-0.5,
		-0.5,
		0.5,
		0.5,
		0.5
	};
	

	mCenteredBuffer->Validate();
	mCenteredBuffer->AssignData(PositionsCentered);

#endif
}

void GraphObject2D::UpdateTexture()
{
#ifndef OLD_GL

	if (!UvBuffer)
		UvBuffer = new VBO(VBO::Dynamic);

	UvBuffer->Validate();
	float CropPositions[12] = { // 2 for each vertex and a uniform for z order
	// topleft
		mCrop_x1,
		mCrop_y1,
	// topright
		mCrop_x2,
		mCrop_y1,
	// bottom right
		mCrop_x2,
		mCrop_y2,
	// topleft again
		mCrop_x1,
		mCrop_y1,
	// bottom left
		mCrop_x1,
		mCrop_y2,
	// bottom right again
		mCrop_x2,
		mCrop_y2
	}; 

	UvBuffer->AssignData(CropPositions);
	DirtyTexture = false;
#endif
}

void Image::Bind()
{
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	LastBound = this;
}

void GraphObject2D::Render()
{
	
	if (mImage)
	{
		mImage->Bind();
	}else
		return;

#ifndef OLD_GL

	if (DirtyMatrix)
	{
		glm::mat4 posMatrix =	glm::scale(
		glm::rotate(
			glm::translate(
					    glm::mat4(1.0f), 
					    glm::vec3(mPosition.x, mPosition.y, 0)), 
					mRotation, glm::vec3(0,0,1)
				   ), glm::vec3(mScale.x*mWidth, mScale.y*mHeight, 1));

		Matrix = posMatrix;
		DirtyMatrix = false;
	}

	if (DirtyTexture)
		UpdateTexture();

	// Assign our matrix.
	WindowFrame.SetUniform("mvp", &Matrix[0][0]);

	// Set the color.
	WindowFrame.SetUniform("Color", Red, Green, Blue, Alpha);
	WindowFrame.SetUniform("inverted", ColorInvert);
	WindowFrame.SetUniform("AffectedByLightning", AffectedByLightning);

	// Assign Texture
	WindowFrame.SetUniform("tex", 0);

	// Assign position attrib. pointer
	if (!Centered)
		mBuffer->Bind();
	else
		mCenteredBuffer->Bind();

	glVertexAttribPointer( WindowFrame.EnableAttribArray("position"), 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0 );

	// assign vertex UVs
	UvBuffer->Bind();
	glVertexAttribPointer( WindowFrame.EnableAttribArray("vertexUV"), 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0 );

	glDrawArrays(GL_TRIANGLES, 0, 6);

	WindowFrame.DisableAttribArray("position");
	WindowFrame.DisableAttribArray("vertexUV");
#else

	if (DirtyMatrix)
	{
		glm::mat4 posMatrix =	glm::scale(
			glm::rotate(
			glm::translate(
			glm::mat4(1.0f), 
			glm::vec3(mPosition.x, mPosition.y, 0)), 
			mRotation, glm::vec3(0,0,1)
			), glm::vec3(mScale.x*mWidth, mScale.y*mHeight, 1));

		Matrix = posMatrix;
		DirtyMatrix = false;
	}

	// old gl code ahead

	if (ColorInvert)
	{
		glLogicOp(GL_INVERT);
		glEnable(GL_COLOR_LOGIC_OP);
	}

	glColor4f(Red, Green, Blue, Alpha);

	glPushMatrix();

	glMultMatrixf(glm::value_ptr(Matrix));

	if (Centered)
		glTranslatef(-0.5, -0.5, 0);

	glBegin(GL_QUADS);

		glTexCoord2f(mCrop_x1, mCrop_y1);
		glVertex2i(0, 0); // topleft
		glTexCoord2f(mCrop_x2, mCrop_y1);
		glVertex2i(1, 0); // topright
		glTexCoord2f(mCrop_x2, mCrop_y2);
		glVertex2i(1, 1); // bottomright
		glTexCoord2f(mCrop_x1, mCrop_y2);
		glVertex2i(0, 1); // bottomleft

	glEnd();
	
	glPopMatrix();

	if (ColorInvert)
		glDisable(GL_COLOR_LOGIC_OP);
#endif
}

void GraphObject2D::Cleanup()
{
#ifndef OLD_GL
	if (DoTextureCleanup)
	{
		delete UvBuffer;
	}
#endif
}

VBO::VBO(Type T)
{
	InternalVBO = 0;
	IsValid = false;
	mType = T;
	WindowFrame.AddVBO(this);
}

VBO::~VBO()
{
	if (InternalVBO)
		glDeleteBuffers(1, &InternalVBO);
	WindowFrame.RemoveVBO(this);
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
		glBindBuffer(GL_ARRAY_BUFFER, InternalVBO);
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

	memmove(VboData, Data, sizeof(VboData));
	Bind();
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 12, VboData, UpType);
}


void VBO::Bind()
{
	glBindBuffer(GL_ARRAY_BUFFER, InternalVBO);
}