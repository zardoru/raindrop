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

using glm::sign;

void ScreenGameplay7K::DrawBarlines()
{
	for (auto i: MeasureBarlines)
	{
		float realV = (CurrentVertical - i) * SpeedMultiplier + BarlineOffset * sign(SpeedMultiplier) + JudgmentLinePos;
		if (realV > 0 && realV < ScreenWidth)
		{
			Barline->SetLocation (Vec2(BarlineX, realV), Vec2(BarlineX + BarlineWidth, realV));
			Barline->Render();
		}
	}
}

Mat4 id;

void ScreenGameplay7K::DrawMeasures()
{
	if (BarlineEnabled)
		DrawBarlines();

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

	SetPrimitiveQuadVBO();

	for (uint32 k = 0; k < CurrentDiff->Channels; k++)
	{
		auto Locate = [&](float StaticVert) -> float {
			return (CurrentVertical - StaticVert) * SpeedMultiplier + JudgmentLinePos;
		};

		/* Find the location of the first/next visible regular note */
		auto LocPredicate = [&](const TrackNote &A, double _) -> bool {
			if (!IsUpscrolling())
				return _ < Locate(A.GetVertical());
			else // Signs are switched. We need to preserve the same order. 
				return _ > Locate(A.GetVertical());
		};

		std::vector<TrackNote>::iterator Start;
		
		// Signs are switched. Doesn't begin by the first note closest to the lower edge, but the one closest to the higher edge.
		if (!IsUpscrolling())
			Start = std::lower_bound(NotesByChannel[k].begin(), NotesByChannel[k].end(), ScreenHeight + Noteskin::GetNoteOffset(), LocPredicate);
		else
			Start = std::lower_bound(NotesByChannel[k].begin(), NotesByChannel[k].end(), 0 - Noteskin::GetNoteOffset(), LocPredicate);

		// Locate the first hold that we can draw in this range
		auto rStart = std::reverse_iterator<vector<TrackNote>::iterator>(Start);
		for (auto i = rStart; i != NotesByChannel[k].rend(); ++i)
		{
			if (i->IsHold() && i->IsVisible()) {
				auto Vert = Locate(i->GetVertical());
				auto VertEnd = Locate(i->GetHoldEndVertical());
				if (IntervalsIntersect(0, ScreenHeight, min(Vert, VertEnd), max(Vert, VertEnd)))
				{
					Start = i.base() - 1;
					break;
				}
			}
		}

		// Find the note that is out of the drawing range
		std::vector<TrackNote>::iterator End;

		// As before. Top becomes bottom, bottom becomes top.
		if (!IsUpscrolling())
			End = std::lower_bound(NotesByChannel[k].begin(), NotesByChannel[k].end(), 0 - Noteskin::GetNoteOffset(), LocPredicate);
		else
			End = std::lower_bound(NotesByChannel[k].begin(), NotesByChannel[k].end(), ScreenHeight + Noteskin::GetNoteOffset(), LocPredicate);
		
		// Now, draw them.
		for (auto m = Start; m != End; ++m)
		{
			float Vertical = 0;
			float VerticalHoldEnd;

			// Don't attempt drawing this object if not visible.
			if (!m->IsVisible())
				continue;

			Vertical = Locate(m->GetVertical());
			VerticalHoldEnd = Locate(m->GetHoldEndVertical());
			
			// Assign our matrix.
			WindowFrame.SetUniform(U_MVP, &id[0][0]);


			float JPos;

			// LR2 style keep-on-the-judgment-line
			bool AboveLine = Vertical < JudgmentLinePos;
			if (!(AboveLine ^ IsUpscrolling()) && m->IsJudgable())
				JPos = JudgmentLinePos;
			else
				JPos = Vertical;

			// We draw the body first, so that way the heads get drawn on top
			if (m->IsHold())
			{
				enum : int {Failed, Active, BeingHit, SuccesfullyHit};
				int Level = -1;

				if (m->IsEnabled() && !m->FailedHit())
					Level = Active;
				if (!m->IsEnabled() && m->FailedHit())
					Level = Failed;
				if (m->IsEnabled() && m->WasNoteHit() && !m->FailedHit())
					Level = BeingHit;
				if (!m->IsEnabled() && m->WasNoteHit() && !m->FailedHit())
					Level = SuccesfullyHit;

				float Pos; 
				float Size; 
				// If we're being hit and..
				if (Noteskin::ShouldDecreaseHoldSizeWhenBeingHit() && Level == 2)
				{
					Pos = (VerticalHoldEnd + JPos) / 2;
					Size = VerticalHoldEnd - JPos;
				}else // We were failed, not being hit or were already hit
				{
					Pos = (VerticalHoldEnd + Vertical) / 2;
					Size = VerticalHoldEnd - Vertical;
				}

				Noteskin::DrawHoldBody(k, Pos, Size, Level);
				Noteskin::DrawHoldTail(*m, k, VerticalHoldEnd, Level);
				

				if (Noteskin::AllowDanglingHeads())
					Noteskin::DrawHoldHead(*m, k, JPos, Level);
				else
					Noteskin::DrawHoldHead(*m, k, Vertical, Level);
			} else
			{
				if (Noteskin::AllowDanglingHeads())
					Noteskin::DrawNote(*m, k, JPos);
				else
					Noteskin::DrawNote(*m, k, Vertical);
			}
		}
	}

	/* Clean up */
	MultiplierChanged = false;
	FinalizeDraw();
}
