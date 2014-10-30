#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp>

#include "GameGlobal.h"
#include "Screen.h"
#include "Audio.h"

#include "GameWindow.h"
#include "Rendering.h"
#include "VBO.h"
#include "GraphObject2D.h"
#include "Line.h"
#include "Image.h"
#include "ImageLoader.h"
#include "ImageList.h"
#include "GraphObjectMan.h"
#include "BitmapFont.h"

#include "ScreenGameplay7K.h"

using namespace VSRG;

static Mat4 identity;

void ScreenGameplay7K::Render()
{
	Background.Render();
	Layer1.Render();
	Layer2.Render();

	if (MissTime > 0)
		LayerMiss.Render();

	Animations->DrawUntilLayer(13);

	DrawMeasures();

	for (int32 i = 0; i < CurrentDiff->Channels; i++)
		Keys[i].Render();

	Animations->DrawFromLayer(14);
}

void ScreenGameplay7K::DrawBarlines(float rPos)
{
	for (std::vector<float>::iterator i = MeasureBarlines.begin();
		i != MeasureBarlines.end();
		i++)
	{
		float realV = rPos - (*i) * SpeedMultiplier + BarlineOffset;
		if (realV > 0 && realV < ScreenWidth)
		{
			Barline->SetLocation (Vec2(BarlineX, realV), Vec2(BarlineX + BarlineWidth, realV));
			Barline->Render();
		}
	}
}

void ScreenGameplay7K::DrawMeasures()
{
	float rPos;
	float MultAbs = abs(SpeedMultiplier);

	rPos = CurrentVertical * SpeedMultiplier + JudgmentLinePos;

	if (BarlineEnabled)
		DrawBarlines(rPos);

	// Set the color.
	WindowFrame.SetUniform(U_INVERT, false); // Color invert
	WindowFrame.SetUniform(U_LIGHT, false); // Affected by lightning
	WindowFrame.SetUniform(U_REPCOLOR, false);

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

	glVertexAttribPointer( WindowFrame.EnableAttribArray(A_UV), 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0 );

	/* todo: instancing */
	for (uint32 k = 0; k < Channels; k++)
	{
		for (std::vector<TrackNote>::iterator m = NotesByChannel[k].begin(); m != NotesByChannel[k].end(); m++)
		{
			if (!m->IsEnabled())
				if (!m->IsHold())
					continue;

			// Is there a better way to do this that doesn't involve recalculating this eeeeeevery note?
			float Vertical = (m->GetVertical() * SpeedMultiplier + rPos) ;
			float VerticalHold;

			bool InScreen = true; 

			if (m->IsHold())
			{
				VerticalHold = (m->GetVerticalHold() * SpeedMultiplier + rPos);

				if (Upscroll)
					InScreen = IntervalsIntersect(0, ScreenHeight, Vertical, VerticalHold);
				else
					InScreen = IntervalsIntersect(0, ScreenHeight, VerticalHold, Vertical);
			}
			else
			{
				if (Upscroll && Vertical > ScreenHeight)
					goto next_key;

				if (!Upscroll && Vertical < 0)
					goto next_key;

				if (Upscroll)
					InScreen = Vertical < ScreenHeight;
				else
					InScreen = Vertical > 0;
			}


			if (!InScreen)
				continue; /* If this is not visible, we move on to the next note. */

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

				WindowFrame.SetUniform(U_TRANM, &(m->GetHoldPositionMatrix(LanePositions[k]))[0][0]);
				WindowFrame.SetUniform(U_SIM, &(m->GetHoldBodyMatrix(LaneWidth[k], MultAbs))[0][0]);
				glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
			}


			// Use the lane's note image

			if (!m->IsHold())
			{
				if (NoteImages[k])
					NoteImages[k]->Bind();
				else
				{
					if (NoteImage)
						NoteImage->Bind();
					else
						continue;
				}
			}


			// Assign the note matrix
			WindowFrame.SetUniform(U_SIM, &(NoteMatrix[k])[0][0]);
			WindowFrame.SetUniform(U_COLOR, 1, 1, 1, 1);

			// Draw Hold tail
			if (m->IsHold())
			{
				if (NoteImagesHoldTail[k])
					NoteImagesHoldTail[k]->Bind();

				WindowFrame.SetUniform(U_TRANM, &(m->GetHoldEndMatrix())[0][0]);
				glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
			}

			// Assign our matrix - encore
			if ( (!m->IsHold() && (Vertical < JudgmentLinePos && Upscroll || Vertical >= JudgmentLinePos && !Upscroll))
				|| (m->IsHold() && (Vertical > VerticalHold && Upscroll || Vertical < VerticalHold && !Upscroll)) )
			{

				// As long as it's not judged, we'll keep it in place 
				WindowFrame.SetUniform(U_MVP, &PositionMatrixJudgment[0][0]);
				WindowFrame.SetUniform(U_TRANM, &(identity)[0][0]);
			}else
			{
				// Otherwise scroll normally
				WindowFrame.SetUniform(U_TRANM, &(m->GetMatrix())[0][0]);
			}

			if (m->IsHold())
				if (NoteImagesHoldHead[k]) NoteImagesHoldHead[k]->Bind();

			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		}

		next_key: (void)0;
	}

	/* Clean up */
	MultiplierChanged = false;
	
	WindowFrame.DisableAttribArray(A_POSITION);
	WindowFrame.DisableAttribArray(A_UV);
}
