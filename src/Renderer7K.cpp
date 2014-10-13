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


	// text
	// This /REALLY REALLY/ shouldn't be done here.
	/*
	if (!Active)
		GFont->DisplayText("press 'enter' to start", Vec2( ScreenWidth / 2 - 23 * 3,ScreenHeight * 5/8));

	for (unsigned int i = 0; i < Channels; i++)
	{
		std::stringstream ss;
		ss << lastClosest[i];
		GFont->DisplayText(ss.str().c_str(), Vec2(floor(Keys[i].GetPosition().x), floor(Keys[i].GetPosition().y)) - Vec2(DigitCount(lastClosest[i]) * 3, 7));
	}


	// speed info

	std::stringstream ss;

	ss << "\nMult/Speed: " << std::setprecision(2) << std::setiosflags(std::ios::fixed) << SpeedMultiplier << "x / " << SpeedMultiplier*4;

	if (SongTime > 0)
		ss << "\nScrolling Speed: " << SectionValue(VSpeeds, SongTime) * SpeedMultiplier;
	else
		ss << "\nScrolling Speed: " << SectionValue(VSpeeds, 0) * SpeedMultiplier;

	if (Auto)
		ss << "\nAuto Mode ";
	else
		ss << "\n";

	ss << "T: " << SongTime << " B: " << CurrentBeat << " O: " << CurrentDiff->Offset;

	GFont->DisplayText(ss.str().c_str(), Vec2(432, ScreenHeight - 145));


	// performance info

	ss.str("");

	ss
	<< "PG: " << score_keeper->getJudgmentCount(SKJ_W1) << "\n"
	<< "GR: " << score_keeper->getJudgmentCount(SKJ_W2) << "\n"
	<< "GD: " << score_keeper->getJudgmentCount(SKJ_W3) << "\n"
	<< "BD: " << score_keeper->getJudgmentCount(SKJ_W4) << "\n"
	<< "NG: " << score_keeper->getJudgmentCount(SKJ_W5) << "\n";

	GFont->DisplayText(ss.str().c_str(), Vec2(432, ScreenHeight - 80));


	ss.str("");

	ss
	// << "EX score: " << score_keeper->getPercentScore(PST_EX) << "\n"
	// << "Rank score: " << score_keeper->getPercentScore(PST_RANK) << "\n"
	<< "Beatmania score: " << score_keeper->getScore(ST_IIDX) << "\n"
	<< "Lunatic Rave 2 score: " << score_keeper->getScore(ST_LR2) << "\n"
	;

	GFont->DisplayText(ss.str().c_str(), Vec2(20, ScreenHeight - 40));
	*/


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


			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

		}

		next_key: (void)0;
	}

	/* Clean up */
	MultiplierChanged = false;
	
	WindowFrame.DisableAttribArray(A_POSITION);
	WindowFrame.DisableAttribArray(A_UV);
}
