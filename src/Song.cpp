#include "Global.h"
#include "Game_Consts.h"
#include "GameObject.h"
#include "Song.h"

#include <boost/foreach.hpp>

// converts bpm into seconds per beat
float spb(float bpm)
{
	return 60 / bpm;
}

Song::Song()
{
}

void Song::Process(bool CalculateXPos)
{
	for(std::vector<SongInternal::Difficulty*>::iterator Difficulty = Difficulties.begin(); Difficulty != Difficulties.end(); Difficulty++ )
	{
		int CurrentMeasure = 0;
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
					it->position.x += ScreenDifference;
			}
			CurrentMeasure++;
		}
	}
}