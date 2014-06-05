#include "GameGlobal.h"
#include "SongDC.h"

#include <fstream>
#include <algorithm>
#include <boost/foreach.hpp>

using std::vector;
using namespace dotcur;

Song::Song()
{
	LeadInTime = 0;
	MeasureLength = 4; // MeasureLength/4
}

Song::~Song()
{
}

void CalculateBarlineRatios(Song &MySong, Difficulty &Diff)
{
	TimingData &Timing = Diff.Timing;
	TimingData &Ratios = Diff.BarlineRatios;
	TimingData ChangesInInterval;

	Ratios.clear();

	for (uint32 Measure = 0; Measure < Diff.Measures.size(); Measure++)
	{
		double CurrentBeat = Measure * MySong.MeasureLength;
		double NextBeat = (Measure+1) * MySong.MeasureLength;
		double endMeasureTime = TimeAtBeat(Diff.Timing, Diff.Offset, (Measure+1)*MySong.MeasureLength);
		double startMeasureTime = TimeAtBeat(Diff.Timing, Diff.Offset, Measure*MySong.MeasureLength);
		double MeasureDuration = endMeasureTime - startMeasureTime;

		GetTimingChangesInInterval(Timing, CurrentBeat, NextBeat, ChangesInInterval);

		if (ChangesInInterval.size() == 0)
		{
			double Ratio = 1/MeasureDuration;
			TimingSegment New;

			New.Value = Ratio;
			New.Time = TimeAtBeat(Diff.Timing, Diff.Offset, CurrentBeat);

			if (!Ratios.size())
				Ratios.push_back(New);
			else
			{
				if (Ratios.back().Value != Ratio) /* Don't add redundant ratios.*/
					Ratios.push_back(New);
			}
		}else
		{
			// Real show time on calculations is here.
			for (TimingData::iterator i = ChangesInInterval.begin(); i != ChangesInInterval.end(); i++)
			{
				TimingSegment New;
				double Duration;
				double DurationSeconds;
				double Fraction;
				double Ratio;

				/* Calculate how long this change lasts. */
				
				if ((i+1) != ChangesInInterval.end())
				{
					Duration = (i+1)->Time - i->Time;
				}else
				{
					Duration = NextBeat - i->Time;
				}

				/* Assign the time the change starts
				which we can assume i->Time >= CurrentBeat before this. */
				CurrentBeat = i->Time;

				/* Calculate how much the barline would move in 1 second */
				Fraction = Duration / MySong.MeasureLength;
				
				/* t/b * b = t */ 
				DurationSeconds = spb(i->Value) * Duration;

				if (DurationSeconds == 0)
					continue;

				/* f/d = r/1 (where 1 and d are 1 second and d seconds) */
				Ratio = Fraction / DurationSeconds;

				/* create new segment at i->Time */
				New.Value = Ratio;
				New.Time = TimeAtBeat(Diff.Timing, Diff.Offset, CurrentBeat);

				if (!Ratios.size())
					Ratios.push_back(New);
				else
				{
					if (Ratios.back().Value != Ratio) /* Don't add redundant ratios.*/
						Ratios.push_back(New);
				}
			}
		}
	}
}

void Song::Repack()
{
	for(vector<dotcur::Difficulty*>::iterator Diff = Difficulties.begin(); Diff != Difficulties.end(); Diff++ )
	{
		for(vector<Measure>::iterator Msr = (*Diff)->Measures.begin(); Msr != (*Diff)->Measures.end(); Msr++)
		{
			for (vector<GameObject>::iterator it = Msr->begin(); it != Msr->end(); it++)
			{
				if (it->GetPosition().x > ScreenDifference)
					it->SetPositionX(it->GetPosition().x - ScreenDifference);
			}
		}
	}
}

int noteSort(const GameObject &A, const GameObject &B)
{
	return A.GetFraction() < B.GetFraction();
}

void Song::Process(bool CalculateXPos)
{
	for(std::vector<dotcur::Difficulty*>::iterator Diff = Difficulties.begin(); Diff != Difficulties.end(); Diff++ )
	{
		int32 CurrentMeasure = 0;
		for(vector<Measure>::iterator Msr = (*Diff)->Measures.begin(); Msr != (*Diff)->Measures.end(); Msr++)
		{
			uint32 CurNote = 0;

			std::sort(Msr->begin(), Msr->end(), noteSort);

			for (std::vector<GameObject>::iterator it = Msr->begin(); it != Msr->end(); it++)
			{
				// all measures are 4/4 (good enough for now, change both 4s in the future, maybe)
				it->beat = ((float)CurrentMeasure * MeasureLength) + ((float)it->Fraction * MeasureLength);

				if (it->hold_duration > 0)
					it->endTime = TimeAtBeat((*Diff)->Timing, (*Diff)->Offset, it->beat + it->hold_duration);

				it->startTime = TimeAtBeat((*Diff)->Timing, (*Diff)->Offset, it->beat);

				(*Diff)->Duration = max(it->startTime, (*Diff)->Duration);

				double frac = it->Fraction;

				it->Green = 0;

				if (CurrentMeasure % 2)
				{
					it->SetPositionY(PlayfieldHeight - (PlayfieldHeight * frac) + ScreenOffset);
					it->Red = 1;
					it->Blue = 0;
				}
				else
				{
					it->SetPositionY(PlayfieldHeight * frac + ScreenOffset);
					it->Red = 0;
					it->Blue = 200.0f / 255.0f;
				}

				if (it->endTime > 0)
				{
					(*Diff)->Duration = max(it->endTime, (*Diff)->Duration);
					it->Green = 0.5;
				}

				if (CalculateXPos)
				{
					if (it->GetPosition().x > 0)
					{
						CurNote++;
						it->SetPositionX(it->GetPosition().x + ScreenDifference);
					}
				}

				it->waiting_time = 0.05f * CurNote;
				it->fadein_time = 0.15f;
			}
			CurrentMeasure++;
		}
		CalculateBarlineRatios(*this, **Diff);
	}
}

int GetFractionKindMeasure(double frac);

bool Song::Save(const char* Filename)
{
	std::ofstream Out(Filename);

	if (!Out.is_open()) // couldn't open file for writing.
	{
		return false;
	}

	Out << "#NAME:" << SongName << ";\n";
	Out << "#SONG:" << SongFilename << ";\n";
	Out << "#AUTHOR:" << SongAuthor << ";\n";
	Out << "#BACKGROUNDIMAGE:" << BackgroundFilename << ";\n";

	if (MeasureLength != 4)
		Out << "#MLEN:" << MeasureLength << ";\n";

	for (std::vector<dotcur::Difficulty*>::iterator i = Difficulties.begin(); i != Difficulties.end(); i++)
	{
		if ((*i)->Timing.size() == 1)
			Out << "#BPM:" << (*i)->Timing[0].Value << ";\n";
		else
		{
			Out << "#BPM:";

			for (uint32 k = 0; k < (*i)->Timing.size(); k++)
			{
				Out << (*i)->Timing[k].Time << "=" << (*i)->Timing[k].Value;
				if (k+1 < (*i)->Timing.size())
					Out << ",";
			}

			Out << ";\n";
		}
		Out << "#OFFSET:" << (*i)->Offset << ";\n";

		if (LeadInTime)
			Out << "#LEADIN:" << LeadInTime << ";\n";

		Out << "#NOTES:";

		int MNum = 0;

		// for each measure of this difficulty
		for (vector<dotcur::Measure>::iterator m = (*i)->Measures.begin(); m != (*i)->Measures.end(); m++)
		{
			dotcur::Measure old = *m; // Copy temporarily
			int mAdvance = 192;

			std::sort(m->begin(), m->end(), noteSort);

			// get lowest fraction that is valid
			for (uint32 n = 0; n != m->size(); n++)
			{
				GameObject &G = (*m)[n];
				if (G.GetPosition().x != 0)
				{
					int advance = 192 / GetFractionKindMeasure(G.GetFraction());
					mAdvance = std::min(advance, mAdvance);
				}
			}

			// for each note of this difficulty

			// For the first note, we must leave a gap so we can pretend there was a note "before"
			// and that gap between a null note and the first note can be filled in
			// if the first note WERE to be fraction == 0
			// limit would be 1, -1 to account for the previous note so it doesn't get filled
			// if it weren't it's going to add one so the limit isn't below what we expect
			int prevRow = -mAdvance; 
			for (uint32 n = 0; n != m->size(); n++)
			{
				if ((*m)[n].GetPosition().x == 0)
					continue;

				// what row is this?
				int nRow = (*m)[n].GetFraction() * 192.0;

				// Fill the gap between previous and current note
				if (nRow)
				{
					// how many rows are between this and the previous note
					int limit = (nRow-prevRow) / mAdvance;

					// at this point a negative limit will be effectively impossible
					limit -= 1; 
					assert(limit >= 0);
					for (int k = 0; k < limit; k++)
						Out << "{0}";
				}

				// Fill the current note.
				Out << "{" << (int)(*m)[n].GetPosition().x;
				
				if ((*m)[n].hold_duration)
					Out << " " << (*m)[n].hold_duration;
					
				Out << "}";

				prevRow = nRow;
			} // For each note

			// fill end
			// total rows from last to end of measure, minus the last note itself
			// it should usually be > 1 when there are notes in the measure
			int limit = (192-prevRow) / mAdvance - 1;
			for (int k = 0; k < limit; k++)
				Out << "{0}";

			Out << ",\n";

			*m = old; // Copy back
			Out.flush();

			MNum++;
		} // For each measure
		Out << ";\n";

	} // For each difficulty

	return true;
}
