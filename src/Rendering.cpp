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

#include <ctime>

void GraphObject2D::Init(bool GenBuffers)
{
#ifndef OLD_GL
	float GLPositions[24]; // 2 for each vertex and an uniform for z order

	// The crop/etc will probably be fine with dynamic draw.
	if (GenBuffers && !IsInitialized)
	{
		glGenBuffers(1, &ourBuffer);
		IsInitialized = true;
	}

	// since shaders are already loaded from graphman's init functions..
	// we'll deal with what we need to deal.
	if (!Centered)
	{
		GLPositions[0] = 0;
		GLPositions[1] = 0;


		GLPositions[4] = width;
		GLPositions[5] = 0;


		GLPositions[8] = width;
		GLPositions[9] = height;

		
		GLPositions[12] = 0;
		GLPositions[13] = 0;

		
		GLPositions[16] = 0;
		GLPositions[17] = height;

		
		GLPositions[20] = width;
		GLPositions[21] = height;
		
	}
	else
	{
		GLPositions[0] = -(float)(width / 2);
		GLPositions[1] = -(float)(height / 2);

		GLPositions[4] = width / 2;
		GLPositions[5] = -(float)(height / 2);

		GLPositions[8] = width / 2;
		GLPositions[9] = height / 2;

		GLPositions[12] = -(float)width / 2;
		GLPositions[13] = -(float)height / 2;

		GLPositions[16] = -(float)width / 2;
		GLPositions[17] = height / 2;

		GLPositions[20] = width / 2;
		GLPositions[21] = height / 2;
	}

	// This doesn't matter when we're talking about the origin.
	// 3, 8, 13, 18, 23, 28
	// topleft
	GLPositions[2] = crop_x1;
	GLPositions[3] = crop_y1;
	// topright
	GLPositions[6] = crop_x2;
	GLPositions[7] = crop_y1;
	// bottom right
	GLPositions[10] = crop_x2;
	GLPositions[11] = crop_y2;
	// topleft again
	GLPositions[14] = crop_x1;
	GLPositions[15] = crop_y1;
	// bottom left
	GLPositions[18] = crop_x1;
	GLPositions[19] = crop_y2;
	// bottom right again
	GLPositions[22] = crop_x2;
	GLPositions[23] = crop_y2;

	glBindBuffer(GL_ARRAY_BUFFER, ourBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 24, GLPositions, GL_STATIC_DRAW);
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
	glm::mat4 posMatrix =	glm::scale(
		glm::rotate(
			glm::translate(glm::mat4(1.0f), glm::vec3(position.x, position.y, 0)), 
		rotation, glm::vec3(0,0,1)
				  ), glm::vec3(scaleX, scaleY, 1));
#ifndef OLD_GL
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
	glVertexAttribPointer( posAttrib, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, 0 );

	// assign vertex UVs
	posAttrib = glGetAttribLocation( GraphMan.GetShaderProgram(), "vertexUV" );
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer( posAttrib, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (GLvoid*)(sizeof(float)*2) );

	// DO IT FAGGOT
	glDrawArrays(GL_TRIANGLES, 0, 6);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
#else

	// old gl code ahead

	glColor4f(red, green, blue, alpha);

	glPushMatrix();

	glMultMatrixf(glm::value_ptr(posMatrix));

	if (!Centered)
	{
	glBegin(GL_QUADS);

		glTexCoord2f(crop_x1, crop_y1);
		glVertex3i(0, 0, z_order); // topleft
		glTexCoord2f(crop_x2, crop_y1);
		glVertex3i(width, 0, z_order); // topright
		glTexCoord2f(crop_x2, crop_y2);
		glVertex3i(width, height, z_order); // bottomright
		glTexCoord2f(crop_x1, crop_y2);
		glVertex3i(0, height, z_order); // bottomleft

	glEnd();
	}else
	{
		glBegin(GL_QUADS);

		glTexCoord2f(crop_x1, crop_y1);
		glVertex3i(-((int32)(width / 2)), -((int32)(height/2)), z_order);
		glTexCoord2f(crop_x2, crop_y1);
		glVertex3i(width/2, -((int32)(height/2)), z_order);
		glTexCoord2f(crop_x2, crop_y2);
		glVertex3i(width/2, height/2, z_order);
		glTexCoord2f(crop_x1, crop_y2);
		glVertex3i(-((int32)(width/2)), height/2, z_order);

		glEnd();
	}
	
	glPopMatrix();
#endif
}
