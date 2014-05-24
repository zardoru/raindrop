#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp>

#include "GameGlobal.h"
#include "Screen.h"
#include "Audio.h"

#include "GameWindow.h"
#include "Rendering.h"
#include "VBO.h"
#include "GraphObject2D.h"
#include "Image.h"
#include "ImageLoader.h"

#include "ScreenGameplay7K.h"

using namespace VSRG;

void ScreenGameplay7K::DrawMeasures()
{
	float rPos;
	float MultAbs = abs(SpeedMultiplier);

	rPos = CurrentVertical * SpeedMultiplier + BasePos;

	// Set the color.
	WindowFrame.SetUniform(U_INVERT, false); // Color invert
	WindowFrame.SetUniform(U_LIGHT, false); // Affected by lightning

	// Sudden = 1, Hidden = 2, flashlight = 3 (Defined in the shader)
	WindowFrame.SetUniform(U_HIDDEN, RealHiddenMode); // Affected by hidden lightning?

	if (RealHiddenMode)
	{
		WindowFrame.SetUniform(U_HIDLOW, HideClampLow);
		WindowFrame.SetUniform(U_HIDHIGH, HideClampHigh);
		WindowFrame.SetUniform(U_HIDFAC, HideClampFactor);
		WindowFrame.SetUniform(U_HIDSUM, HideClampSum);
	}
	
	WindowFrame.SetUniform(U_TRANSL, true); // use extra matrices
	WindowFrame.SetUniform(U_CENTERED, true); // center vertexes

	WindowFrame.SetUniform(U_SMULT, SpeedMultiplier);

	GraphObject2D::BindTopLeftVBO();
	glVertexAttribPointer( WindowFrame.EnableAttribArray(A_POSITION), 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0 );

	// Ugly hack for now.
	Background.BindTextureVBO(); 
	glVertexAttribPointer( WindowFrame.EnableAttribArray(A_UV), 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0 );
	

	/* todo: instancing */
	for (uint32 k = 0; k < Channels; k++)
	{
		MeasureVectorTN &Measures = NotesByMeasure[k];

		for (MeasureVectorTN::iterator i = Measures.begin(); i != Measures.end(); i++)
		{
			for (std::vector<TrackNote>::iterator m = i->begin(); m != i->end(); m++)
			{
				float Vertical = (m->GetVertical() * SpeedMultiplier + rPos) ;
				float VerticalHold = (m->GetVerticalHold() * SpeedMultiplier + rPos) ;

				if (MultiplierChanged && m->IsHold())
					m->RecalculateBody(LanePositions[k], LaneWidth[k], NoteHeight, MultAbs);

				bool InScreen = true; 

				if (m->IsHold())
				{
					if (Upscroll)
					{
						InScreen = IntervalsIntersect(0, ScreenHeight, Vertical, VerticalHold);
					}else
						InScreen = IntervalsIntersect(0, ScreenHeight, VerticalHold, Vertical);
				}
				else
				{
					if (Upscroll)
						InScreen = Vertical < ScreenHeight;
					else
						InScreen = Vertical > 0;
				}
				

				if (!InScreen)
					continue; /* If this is not visible, we move on to the next key. */

				// Assign our matrix.
				WindowFrame.SetUniform(U_MVP, &PositionMatrix[0][0]);

				// We draw the body first, so that way the heads get drawn on top
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

				if ( m->IsEnabled() || m->IsHold() ) // Hold head or enabled note
				{
					// Use the lane's note image
					if (NoteImages[k])
						NoteImages[k]->Bind();
					else
					{
						if (NoteImage)
							NoteImage->Bind();
						else
							continue;
					}

					// Assign the note matrix
					WindowFrame.SetUniform(U_SIM, &(NoteMatrix[k])[0][0]);
					WindowFrame.SetUniform(U_COLOR, 1, 1, 1, 1);

					// Draw Hold tail
					if (m->IsHold())
					{
						WindowFrame.SetUniform(U_TRANM, &(m->GetHoldEndMatrix())[0][0]);
						glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
					}

					// Assign our matrix - encore
					if ( (Vertical < BasePos && Upscroll) || (Vertical >= BasePos && !Upscroll) )
					{
						// As long as it's not judged, we'll keep it in place 
						Mat4 identity;
						WindowFrame.SetUniform(U_MVP, &PositionMatrixJudgement[0][0]);
						WindowFrame.SetUniform(U_TRANM, &(identity)[0][0]);
					}else
					{
						// Otherwise scroll normally
						WindowFrame.SetUniform(U_TRANM, &(m->GetMatrix())[0][0]);
					}


					glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
				}
			}
		}
	}

	/* Clean up */
	MultiplierChanged = false;
	
	WindowFrame.DisableAttribArray(A_POSITION);
	WindowFrame.DisableAttribArray(A_UV);
}
