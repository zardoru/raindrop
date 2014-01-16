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
	typedef std::vector<SongInternal::Measure<TrackNote> > NoteVector;
	float rPos;

	if (MultiplierChanged)
	{
		if (Upscroll)
			SpeedMultiplier = - (SpeedMultiplierUser + waveEffect);
		else
			SpeedMultiplier = SpeedMultiplierUser + waveEffect;
	}

	rPos = CurrentVertical * SpeedMultiplier + BasePos;

	// Assign our matrix.
	WindowFrame.SetUniform(U_MVP, &PositionMatrix[0][0]);

	// Set the color.
	WindowFrame.SetUniform(U_INVERT, false); // Color invert
	WindowFrame.SetUniform(U_LIGHT, false); // Affected by lightning
	
	WindowFrame.SetUniform(U_TRANSL, true); // use extra matrices
	WindowFrame.SetUniform(U_CENTERED, true); // center vertexes

	WindowFrame.SetUniform(U_SMULT, SpeedMultiplier);

	GraphObject2D::BindTopLeftVBO();

	glVertexAttribPointer( WindowFrame.EnableAttribArray(A_POSITION), 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0 );

	/* todo: instancing */
	for (uint32 k = 0; k < Channels; k++)
	{
		NoteVector &Measures = NotesByMeasure[k];

		for (NoteVector::iterator i = Measures.begin(); i != Measures.end(); i++)
		{
			for (std::vector<TrackNote>::iterator m = (*i).MeasureNotes.begin(); m != (*i).MeasureNotes.end(); m++)
			{
				float Vertical = (m->GetVertical() * SpeedMultiplier + rPos) ;
				float VerticalHold = (m->GetVerticalHold() * SpeedMultiplier + rPos) ;

				if (MultiplierChanged && m->IsHold())
					m->RecalculateBody(LanePositions[m->GetTrack()], LaneWidth[m->GetTrack()], NoteHeight, Upscroll? -SpeedMultiplier : SpeedMultiplier);

				bool InScreen = true; 

				if (m->IsHold())
				{
					if (Upscroll)
					{
						InScreen = IntervalsIntersect(0, ScreenHeight, Vertical, VerticalHold);
					}else
						InScreen = IntervalsIntersect(0, ScreenHeight, VerticalHold, Vertical);
				}else
				{
					if (Upscroll)
						InScreen = Vertical < ScreenHeight;
					else
						InScreen = Vertical > 0;
				}
				

				if (!InScreen)
					continue; /* If this is not visible, we move on to the next key. */


				if (m->IsHold())
				{
					if (NoteImagesHold[k])
						NoteImagesHold[k]->Bind();
					else
					{
						if (NoteImage)
							NoteImage->Bind();
						else
							continue;
					}

					if (m->IsEnabled())
						WindowFrame.SetUniform(U_COLOR, 1, 1, 1, 1);
					else
						WindowFrame.SetUniform(U_COLOR, 0.5, 0.5, 0.5, 1);

					WindowFrame.SetUniform(U_SIM, &(m->GetHoldBodySizeMatrix())[0][0]);
					WindowFrame.SetUniform(U_TRANM, &(m->GetHoldBodyMatrix())[0][0]);
					glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
				}

				if (NoteImages[k])
					NoteImages[k]->Bind();
				else
				{
					if (NoteImage)
						NoteImage->Bind();
					else
						continue;
				}

				WindowFrame.SetUniform(U_COLOR, 1, 1, 1, 1);

				WindowFrame.SetUniform(U_SIM, &(NoteMatrix[m->GetTrack()])[0][0]);

				WindowFrame.SetUniform(U_TRANM, &(m->GetMatrix())[0][0]);
				glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

				if (m->IsHold())
				{
					WindowFrame.SetUniform(U_TRANM, &(m->GetHoldEndMatrix())[0][0]);
					glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
				}
			}
		}
	}

	/* Clean up */
	MultiplierChanged = false;
	
	WindowFrame.DisableAttribArray(A_POSITION);
}
