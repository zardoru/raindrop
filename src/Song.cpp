#include "Global.h"
#include "GameObject.h"
#include "Song.h"

#include <fstream>
#include <boost/foreach.hpp>

SongInternal::Measure::Measure()
{
	Fraction = 0;
}

Song::Song()
{
	LeadInTime = 0;
	MeasureLength = 4; // MeasureLength/4
}

Song::~Song()
{
}

void Song::Repack()
{
	for(std::vector<SongInternal::Difficulty*>::iterator Difficulty = Difficulties.begin(); Difficulty != Difficulties.end(); Difficulty++ )
	{
		for(std::vector<SongInternal::Measure>::iterator Measure = (*Difficulty)->Measures.begin(); Measure != (*Difficulty)->Measures.end(); Measure++)
		{
			for (std::vector<GameObject>::iterator it = Measure->MeasureNotes.begin(); it != Measure->MeasureNotes.end(); it++)
			{
				if (it->GetPosition().x > ScreenDifference)
					it->SetPositionX(it->GetPosition().x - ScreenDifference);
			}
		}
	}
}

void Song::Process(bool CalculateXPos)
{
	for(std::vector<SongInternal::Difficulty*>::iterator Difficulty = Difficulties.begin(); Difficulty != Difficulties.end(); Difficulty++ )
	{
		int32 CurrentMeasure = 0;
		for(std::vector<SongInternal::Measure>::iterator Measure = (*Difficulty)->Measures.begin(); Measure != (*Difficulty)->Measures.end(); Measure++)
		{
			for (std::vector<GameObject>::iterator it = Measure->MeasureNotes.begin(); it != Measure->MeasureNotes.end(); it++)
			{
				// all measures are 4/4 (good enough for now, change both 4s in the future, maybe)
				it->beat = ((float)CurrentMeasure * MeasureLength) + ((float)it->MeasurePos * MeasureLength) / (float)Measure->Fraction;

				if (it->hold_duration > 0)
				{
					it->endTime = TimeAtBeat(**Difficulty, it->beat + it->hold_duration);
				}

				it->startTime = TimeAtBeat(**Difficulty, it->beat);

				float frac = float(it->MeasurePos) / float(Measure->Fraction);

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
					it->Green = 0.5;

				if (CalculateXPos)
				{
					if (it->GetPosition().x > 0)
						it->SetPositionX(it->GetPosition().x + ScreenDifference);
				}
			}
			CurrentMeasure++;
		}
		(*Difficulty)->Duration = DifficultyDuration(*this, **Difficulty);
		CalculateBarlineRatios(*this, **Difficulty);
	}
}

void Song::Save(const char* Filename)
{
	std::ofstream Out(Filename);

	if (!Out.is_open()) // couldn't open file for writing.
	{
		return /* false */;
	}

	Out << "#NAME:" << SongName << ";\n";
	Out << "#SONG:" << SongRelativePath << ";\n";
	Out << "#AUTHOR:" << SongAuthor << ";\n";
	Out << "#BACKGROUNDIMAGE:" << BackgroundRelativeDir << ";\n";

	if (MeasureLength != 4)
		Out << "#MLEN:" << MeasureLength << ";\n";

	for (std::vector<SongInternal::Difficulty*>::iterator i = Difficulties.begin(); i != Difficulties.end(); i++)
	{
		if ((*i)->Timing.size() == 1)
			Out << "#BPM:" << (*i)->Timing[0].Value << ";\n";
		else
		{
			Out << "#BPM:";

			for (int k = 0; k < (*i)->Timing.size(); k++)
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
		// for each measure of this difficulty
		for (std::vector<SongInternal::Measure>::iterator m = (*i)->Measures.begin(); m != (*i)->Measures.end(); m++)
		{
			// for each note of this difficulty
			for (uint32 n = 0; n != m->MeasureNotes.size(); n++)
			{
				// Fill in the blanks between the first and second notes.
				if (n == 0)
				{
					if (m->MeasureNotes[n].MeasurePos != 0)
					{
						for (uint32 i = 0; i < m->MeasureNotes[n].MeasurePos; i++)
						{
							Out << "{0}";
						}
					}
				}

				// Fill the current note.
				Out << "{" << (int)m->MeasureNotes[n].GetPosition().x;
				
				if (m->MeasureNotes[n].hold_duration)
					Out << " " << m->MeasureNotes[n].hold_duration;
					
				Out << "}";

				// Fill in the blanks between two notes.

				if (n+1 < m->MeasureNotes.size())
				{
					int32 Difference = (m->MeasureNotes.at(n+1).MeasurePos - m->MeasureNotes.at(n).MeasurePos);
					if (Difference > 1)
					{
						while (Difference > 1)
						{
							Out << "{0}";
							Difference--;
						}
					}
				}
				

				if (n == m->MeasureNotes.size()-1) // We're at the last note of the measure
				{
					// We have a gap to close between this and the last fraction of the measure?
					if (m->MeasureNotes[n].MeasurePos < m->Fraction-1)
					{
						// Close it.
						int32 Difference = m->Fraction-1 - m->MeasureNotes[n].MeasurePos;
						while (Difference > 0)
						{
							Out << "{0}";
							Difference--;
						}
					}
				}

				
			} // For each note

			Out << ",\n";
			Out.flush();
		} // For each measure
		Out << ";\n";

	} // For each difficulty

	/* return true; */
}