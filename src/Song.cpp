#include "Global.h"
#include "Game_Consts.h"
#include "GameObject.h"
#include "Song.h"

#include <fstream>
#include <boost/foreach.hpp>

// converts bpm into seconds per beat
float spb(float bpm)
{
	return 60 / bpm;
}

Song::Song()
{
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
				if (it->position.x > ScreenDifference)
					it->position.x -= ScreenDifference;
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
				it->beat = ((float)CurrentMeasure * 4.0) + ((float)it->MeasurePos * 4.0) / (float)Measure->Fraction;

				if (it->hold_duration > 0)
				{
					it->endTime = (it->beat + it->hold_duration) * spb((*Difficulty)->Timing.at(0).Value) + (*Difficulty)->Offset;
				}

				it->startTime = it->beat * spb ((*Difficulty)->Timing.at(0).Value) + (*Difficulty)->Offset;

				float frac = float(it->MeasurePos) / float(Measure->Fraction);

				it->green = 0;

				if (CurrentMeasure % 2)
				{
					it->position.y = PlayfieldHeight - (PlayfieldHeight * frac) + ScreenOffset;
					it->red = 1;
					it->blue = 0;
				}
				else
				{
					it->position.y = PlayfieldHeight * frac + ScreenOffset;
					it->red = 0;
					it->blue = 200.0f / 255.0f;
				}

				if (it->endTime > 0)
					it->green = 0.5;
				it->Init(false);

				if (CalculateXPos)
				{
					if (it->position.x > 0)
						it->position.x += ScreenDifference;
				}
			}
			CurrentMeasure++;
		}
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
	Out << "#SONG:" << SongFilename << ";\n";
	Out << "#BACKGROUNDIMAGE:" << BackgroundDir << ";\n";

	for (std::vector<SongInternal::Difficulty*>::iterator i = Difficulties.begin(); i != Difficulties.end(); i++)
	{
		Out << "#BPM:" << (*i)->Timing[0].Value << ";\n";
		Out << "#OFFSET:" << (*i)->Offset << ";\n";

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
				Out << "{" << m->MeasureNotes[n].position.x << "}";

				// Fill in the blanks between two notes.
				try
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
				}catch (...) {}

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

			Out << ",";
			Out.flush();
		} // For each measure
		Out << ";\n";

	} // For each difficulty

	/* return true; */
}