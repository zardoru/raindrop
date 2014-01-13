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

inline bool IntervalsIntersect(const double a, const double b, const double c, const double d)
{
	return a <= d && c <= b;
}

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
	glUniformMatrix4fv(0, 1, GL_FALSE, &PositionMatrix[0][0]);

	// Set the color.
	glUniform1i(9, false); // Color invert
	glUniform1i(10, false); // Affected by lightning
	
	glUniform1i(3, true); // use extra matrices
	glUniform1i(4, true); // center vertexes

	glUniform1f(5, SpeedMultiplier);

	GraphObject2D::BindTopLeftVBO();

	glEnableVertexAttribArray(0);
	glVertexAttribPointer( 0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0 );

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
					m->RecalculateBody(GearLaneWidth, 10, Upscroll? -SpeedMultiplier : SpeedMultiplier);

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
						glUniform4f(6, 1, 1, 1, 1);
					else
						glUniform4f(6, 0.5, 0.5, 0.5, 1);

					glUniformMatrix4fv(2, 1, GL_FALSE, &(m->GetHoldBodySizeMatrix())[0][0]);
					glUniformMatrix4fv(1, 1, GL_FALSE, &(m->GetHoldBodyMatrix())[0][0]);
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

				glUniform4f(6, 1, 1, 1, 1);

				glUniformMatrix4fv(2, 1, GL_FALSE, &(NoteMatrix)[0][0]);

				glUniformMatrix4fv(1, 1, GL_FALSE, &(m->GetMatrix())[0][0]);
				glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

				if (m->IsHold())
				{
					glUniformMatrix4fv(1, 1, GL_FALSE, &(m->GetHoldEndMatrix())[0][0]);
					glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
				}
			}
		}
	}

	/* Clean up */
	MultiplierChanged = false;
	
	glDisableVertexAttribArray(11);
}

void ScreenGameplay7K::DrawExplosions()
{
	for (int i = 0; i < CurrentDiff->Channels; i++)
	{
		int Frame = ExplosionTime[i] / 0.016;

		if (Frame > 19)
			Explosion[i].Alpha = 0;
		else
		{
			Explosion[i].Alpha = 1;
			Explosion[i].SetImage ( ExplosionFrames[Frame], false );
		}

		Explosion[i].Render();

	}
}
