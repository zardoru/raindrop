#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp>

#include "Global.h"
#include "Screen.h"
#include "Audio.h"

#include "GameWindow.h"
#include "Rendering.h"
#include "VBO.h"
#include "GraphObject2D.h"
#include "Image.h"
#include "ImageLoader.h"

#include "Song.h"
#include "ScreenGameplay7K.h"



void ScreenGameplay7K::DrawMeasures()
{
	NoteImage->Bind();
	// Assign our matrix.
	WindowFrame.SetUniform("mvp", &PositionMatrix[0][0]);

	// Set the color.
	WindowFrame.SetUniform("Color", 1, 1, 1, 1);
	WindowFrame.SetUniform("inverted", false);
	WindowFrame.SetUniform("AffectedByLightning", false);
	WindowFrame.SetUniform("useTranslate", true);
	WindowFrame.SetUniform("Centered", true);
	GraphObject2D::BindTopLeftVBO();

	glVertexAttribPointer( WindowFrame.EnableAttribArray("position"), 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0 );

	/* todo: instancing */
	for (uint32 k = 0; k < Channels; k++)
	{
		for (uint32 m = 0; m < NotesByMeasure[k].size(); m++)
		{
			for (uint32 q = 0; q < NotesByMeasure[k][m].MeasureNotes.size(); q++)
			{
				if ((NotesByMeasure[k][m].MeasureNotes[q].GetVertical() + CurrentVertical) < 0)
					continue;
				if ((NotesByMeasure[k][m].MeasureNotes[q].GetVertical() + CurrentVertical) > ScreenHeight)
					continue;

				WindowFrame.SetUniform("tranM", &(NotesByMeasure[k][m].MeasureNotes[q].GetMatrix())[0][0]);
				glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
			}
		}
	}

	WindowFrame.DisableAttribArray("position");
}