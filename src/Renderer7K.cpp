#include "GameGlobal.h"
#include "Screen.h"

#include "GameWindow.h"
#include "Rendering.h"
#include "Sprite.h"
#include "Line.h"
#include "SceneEnvironment.h"

#include "ScreenGameplay7K.h"
#include "Noteskin.h"

using namespace VSRG;

void ScreenGameplay7K::Render()
{
	BGA->Render();

	Animations->DrawUntilLayer(13);

	DrawMeasures();

	Animations->DrawFromLayer(14);
}

void ScreenGameplay7K::DrawBarlines(float rPos)
{
	for (auto i = MeasureBarlines.begin();
		i != MeasureBarlines.end();
		++i)
	{
		float realV = rPos - (*i) * SpeedMultiplier + BarlineOffset;
		if (realV > 0 && realV < ScreenWidth)
		{
			Barline->SetLocation (Vec2(BarlineX, realV), Vec2(BarlineX + BarlineWidth, realV));
			Barline->Render();
		}
	}
}

bool ShouldDrawNoteInScreen(TrackNote& T, float SpeedMultiplier, float FieldDisplacement, float &Vertical, float &VerticalHoldEnd, bool Upscroll)
{
	if (!T.IsVisible())
		return false;

	if (!T.IsEnabled())
		if (!T.IsHold())
			return false;

	Vertical = (T.GetVertical() * SpeedMultiplier + FieldDisplacement);
	if (T.IsHold())
	{
		VerticalHoldEnd = (T.GetHoldEndVertical() * SpeedMultiplier + FieldDisplacement);

		if (Upscroll)
			return IntervalsIntersect(0, ScreenHeight, Vertical, VerticalHoldEnd);
		else
			return IntervalsIntersect(0, ScreenHeight, VerticalHoldEnd, Vertical);
	}
	
	if (Upscroll)
		return Vertical < ScreenHeight;
	else
		return Vertical > 0;
}

Mat4 id;

void ScreenGameplay7K::DrawMeasures()
{
	float FieldDisplacement;

	FieldDisplacement = CurrentVertical * SpeedMultiplier + JudgmentLinePos;

	if (BarlineEnabled)
		DrawBarlines(FieldDisplacement);

	// Set some parameters...
	SetShaderParameters(false, false, true, true, false, false, RealHiddenMode);

	// Sudden = 1, Hidden = 2, flashlight = 3 (Defined in the shader)
	if (RealHiddenMode)
	{
		WindowFrame.SetUniform(U_HIDLOW, HideClampLow);
		WindowFrame.SetUniform(U_HIDHIGH, HideClampHigh);
		WindowFrame.SetUniform(U_HIDFAC, HideClampFactor);
		WindowFrame.SetUniform(U_HIDSUM, HideClampSum);
	}

	WindowFrame.SetUniform(U_SIM, &id[0][0]);
	WindowFrame.SetUniform(U_TRANM, &id[0][0]);
	WindowFrame.SetUniform(U_SMULT, SpeedMultiplier);

	SetPrimitiveQuadVBO();

	for (uint32 k = 0; k < CurrentDiff->Channels; k++)
	{
		for (auto m = NotesByChannel[k].begin(); m != NotesByChannel[k].end(); ++m)
		{
			// Is there a better way to do this that doesn't involve recalculating this every note?
			float Vertical = 0;
			float VerticalHoldEnd;

			if (!ShouldDrawNoteInScreen(*m, SpeedMultiplier, FieldDisplacement, Vertical, VerticalHoldEnd, Upscroll))
				continue; /* If this is not visible, we move on to the next note. */

			// Assign our matrix.
			WindowFrame.SetUniform(U_MVP, &id[0][0]);

			// We draw the body first, so that way the heads get drawn on top
			if (m->IsHold())
			{
				float Pos = (VerticalHoldEnd + Vertical) / 2;
				float Size = m->GetHoldSize() * SpeedMultiplier;
				int Level = 1;

				if (m->IsEnabled() && m->WasNoteHit())
					Level = 2;
				if (!m->IsEnabled())
					Level = 0;

				Noteskin::DrawHoldBody(k, Pos, Size, Level);
				Noteskin::DrawHoldTail(*m, k, VerticalHoldEnd);
				
				// LR2 style keep-on-the-judgment-line
				if ( (Vertical > VerticalHoldEnd && Upscroll || Vertical < VerticalHoldEnd && !Upscroll) && m->IsJudgable() )
					Noteskin::DrawHoldHead(*m, k, JudgmentLinePos);
				else
					Noteskin::DrawHoldHead(*m, k, Vertical);
			} else
			{
				if ((Vertical < JudgmentLinePos && Upscroll || Vertical >= JudgmentLinePos && !Upscroll) && m->IsJudgable())
					Noteskin::DrawNote(*m, k, JudgmentLinePos);
				else
					Noteskin::DrawNote(*m, k, Vertical);
			}
		}
	}

	/* Clean up */
	MultiplierChanged = false;
	FinalizeDraw();
}
