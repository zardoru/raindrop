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

void GraphObject2D::Init()
{
#ifndef OLD_GL
	float GLPositions[12]; // 2 for each vertex and an uniform for z order

	// The crop/etc will probably be fine with dynamic draw.
	if (!IsInitialized)
	{
		glGenBuffers(1, &ourBuffer);
		IsInitialized = true;
	}

	// since shaders are already loaded from graphman's init functions..
	// we'll deal with what we need to deal.
	// Our initial quad.
	GLPositions[0] = 0;
	GLPositions[1] = 0;

	GLPositions[2] = 1;
	GLPositions[3] = 0;

	GLPositions[4] = 1;
	GLPositions[5] = 1;

	GLPositions[6] = 0;
	GLPositions[7] = 0;

	GLPositions[8] = 0;
	GLPositions[9] = 1;

	GLPositions[10] = 1;
	GLPositions[11] = 1;

	glBindBuffer(GL_ARRAY_BUFFER, ourBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 12, GLPositions, GL_STATIC_DRAW);

#endif
}

void GraphObject2D::InitTexture()
{
#ifndef OLD_GL

	if (!IsInitialized)
		Init();

	glGenBuffers(1, &ourUVBuffer);

	UpdateTexture();
#endif
}

void GraphObject2D::UpdateTexture()
{
#ifndef OLD_GL
	float GLPositions[12]; // 2 for each vertex and an uniform for z order

	// topleft
	GLPositions[0] = crop_x1;
	GLPositions[1] = crop_y1;
	// topright
	GLPositions[2] = crop_x2;
	GLPositions[3] = crop_y1;
	// bottom right
	GLPositions[4] = crop_x2;
	GLPositions[5] = crop_y2;
	// topleft again
	GLPositions[6] = crop_x1;
	GLPositions[7] = crop_y1;
	// bottom left
	GLPositions[8] = crop_x1;
	GLPositions[9] = crop_y2;
	// bottom right again
	GLPositions[10] = crop_x2;
	GLPositions[11] = crop_y2;

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

		glm::mat4 posMatrix =	glm::scale(
		glm::rotate(
			glm::translate(glm::mat4(1.0f), 
			glm::vec3(!Centered ? position.x : position.x-width*scaleX/2, !Centered? position.y : position.y-height*scaleY/2, 0)), 
		rotation, glm::vec3(0,0,1)
				  ), glm::vec3(scaleX*width, scaleY*height, 1));

	// Assign our matrix.
	GLuint MatrixID = glGetUniformLocation(GraphMan.GetShaderProgram(), "mvp");
	glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &posMatrix[0][0]);

	// Set the color.
	GLuint ColorID = glGetUniformLocation(GraphMan.GetShaderProgram(), "Color");
	glUniform4f(ColorID, red, green, blue, alpha);

	// Draw the buffer.
	
	// assign Texture
	GLint posAttrib = glGetUniformLocation( GraphMan.GetShaderProgram(), "tex" );
	glUniform1i(posAttrib, 0);


	// assign position attrib. pointer
	posAttrib = glGetAttribLocation( GraphMan.GetShaderProgram(), "position" );
	glEnableVertexAttribArray(posAttrib);

	glBindBuffer(GL_ARRAY_BUFFER, ourBuffer);
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
			glm::vec3(!Centered ? position.x : position.x-width*scaleX/2, !Centered? position.y : position.y-height*scaleY/2, 0)), 
		rotation, glm::vec3(0,0,1)
				  ), glm::vec3(scaleX*width, scaleY*height, 1));

	// old gl code ahead

	glColor4f(red, green, blue, alpha);

	glPushMatrix();

	glMultMatrixf(glm::value_ptr(posMatrix));

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
