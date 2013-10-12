#include "Global.h"
#include "Rendering.h"
#include "GraphObject2D.h"

#ifdef WIN32
#include <Windows.h>
#endif
#include <GL/glew.h>
#include "Image.h"
#include <glm/gtc/type_ptr.hpp>
#include "GraphicsManager.h"

uint32 GraphObject2D::ourBuffer;
uint32 GraphObject2D::ourCenteredBuffer;

void GraphObject2D::Init()
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

	// The crop/etc will probably be fine with dynamic draw.
	if (!IsInitialized)
	{
		glGenBuffers(1, &ourBuffer);
		glGenBuffers(1, &ourCenteredBuffer);
		IsInitialized = true;
	}

	// since shaders are already loaded from graphman's init functions..
	// we'll deal with what we need to deal.
	// Our initial quad.

	glBindBuffer(GL_ARRAY_BUFFER, ourBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 12, Positions, GL_STATIC_DRAW);

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
	

	glBindBuffer(GL_ARRAY_BUFFER, ourCenteredBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 12, PositionsCentered, GL_STATIC_DRAW);

#endif
}

void GraphObject2D::InitTexture()
{
#ifndef OLD_GL

	if (!IsInitialized)
		Init();

	if (ourUVBuffer == -1)
		glGenBuffers(1, &ourUVBuffer);

	UpdateTexture();
#endif
}

void GraphObject2D::UpdateTexture()
{
#ifndef OLD_GL
	float GLPositions[12] = { // 2 for each vertex and a uniform for z order
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

	glBindBuffer(GL_ARRAY_BUFFER, ourUVBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 12, GLPositions, GL_DYNAMIC_DRAW);
#endif
}

void GraphObject2D::Render()
{
	
	if (mImage)
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, mImage->texture);
	}
	else
	{
		//root->CONSOLE->PrintLog("Trying to render null texture\n");
		return;
	}

	// todo: not recalculate this every frame lol
	/*glm::mat4 posMatrix =	glm::scale(
		glm::rotate(
			glm::translate(glm::mat4(1.0f), glm::vec3(position.x, position.y, 0)), 
		rotation, glm::vec3(0,0,1)
				  ), glm::vec3(scaleX, scaleY, 1));*/
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
	GLuint MatrixID = glGetUniformLocation(GraphMan.GetShaderProgram(), "mvp");
	glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &Matrix[0][0]);

	// Set the color.
	GLuint ColorID = glGetUniformLocation(GraphMan.GetShaderProgram(), "Color");
	glUniform4f(ColorID, Red, Green, Blue, Alpha);

	GLuint ColorInvertedID = glGetUniformLocation(GraphMan.GetShaderProgram(), "inverted");
	glUniform1i(ColorInvertedID, ColorInvert);

	// Draw the buffer.
	
	// assign Texture
	GLint posAttrib = glGetUniformLocation( GraphMan.GetShaderProgram(), "tex" );
	glUniform1i(posAttrib, 0);


	// assign position attrib. pointer
	posAttrib = glGetAttribLocation( GraphMan.GetShaderProgram(), "position" );
	glEnableVertexAttribArray(posAttrib);

	if (!Centered)
		glBindBuffer(GL_ARRAY_BUFFER, ourBuffer);
	else
		glBindBuffer(GL_ARRAY_BUFFER, ourCenteredBuffer);

	glVertexAttribPointer( posAttrib, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0 );

	// assign vertex UVs
	posAttrib = glGetAttribLocation( GraphMan.GetShaderProgram(), "vertexUV" );
	glEnableVertexAttribArray(posAttrib);

	glBindBuffer(GL_ARRAY_BUFFER, ourUVBuffer);
	glVertexAttribPointer( posAttrib, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0 );

	glBindBuffer(GL_ARRAY_BUFFER, ourBuffer);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
#else
	glm::mat4 posMatrix =	glm::scale(
		glm::rotate(
			glm::translate(glm::mat4(1.0f), 
			glm::vec3(position.x, position.y, 0)), 
		rotation, glm::vec3(0, 0, 1)
				  ), glm::vec3(scaleX*width, scaleY*height, 1));

	// old gl code ahead

	glColor4f(red, green, blue, alpha);

	glPushMatrix();

	glMultMatrixf(glm::value_ptr(posMatrix));

	if (Centered)
		glTranslatef(-0.5, -0.5, 0);

	glBegin(GL_QUADS);

		glTexCoord2f(crop_x1, crop_y1);
		glVertex3i(0, 0, z_order); // topleft
		glTexCoord2f(crop_x2, crop_y1);
		glVertex3i(1, 0, z_order); // topright
		glTexCoord2f(crop_x2, crop_y2);
		glVertex3i(1, 1, z_order); // bottomright
		glTexCoord2f(crop_x1, crop_y2);
		glVertex3i(0, 1, z_order); // bottomleft

	glEnd();
	
	glPopMatrix();
#endif
}

void GraphObject2D::Cleanup()
{
	if (DoTextureCleanup)
	{
		// todo: clean up UV vbo
	}
}