#include "Global.h"
#include "Game_Consts.h"
#include "GameObject.h"
#include "Song.h"

// converts bpm into seconds per beat
float spb(float bpm)
{
	return 60 / bpm;
}

Song::Song()
{
	MeasureCount = BPM = Offset = 0;
	Notes = NULL;
}

void Song::Process()
{
	int CurrentMeasure = 0;
	unsigned int MaxMeasureFrac = 0;

	for (CurrentMeasure = 0; CurrentMeasure < MeasureCount; CurrentMeasure++)
	{
		for (std::vector<GameObject>::iterator it = Notes->begin(); it != Notes->end(); it++)
		{
			if (it->Measure != CurrentMeasure)
				continue;
			MaxMeasureFrac = std::max(MaxMeasureFrac, it->MeasurePos + 1);
		}

		for (std::vector<GameObject>::iterator it = Notes->begin(); it != Notes->end(); it++)
		{
			if (it->Measure != CurrentMeasure)
				continue;

			// all measures are 4/4 (good enough for now, change both 4s in the future, maybe)
			it->beat = ((float)CurrentMeasure * 4.0) + ((float)it->MeasurePos * 4.0) / (float)MaxMeasureFrac;

			if (it->hold_duration > 0)
			{
				it->endTime = (it->beat + it->hold_duration) * spb(BPM) + Offset;
			}

			it->startTime = it->beat * spb (BPM) + Offset;

			float frac = float(it->MeasurePos) / float(MaxMeasureFrac);

			it->green = 0;

			if (CurrentMeasure % 2)
			{
				it->position.y = PlayfieldHeight - (PlayfieldHeight * frac) - 10 + ScreenOffset;
				it->red = 1;
				it->blue = 0;
			}
			else
			{
				it->position.y = PlayfieldHeight * frac + 10 + ScreenOffset;
				it->red = 0;
				it->blue = 200.0 / 255.0;
			}

			if (it->endTime > 0)
				it->green = 0.5;
			it->Init(false);
		}
		MaxMeasureFrac = 0;
	}

}

std::vector<GameObject> Song::GetObjectsForMeasure(int measure)
{
	std::vector <GameObject> retval;

	for (std::vector<GameObject>::iterator i = Notes->begin(); i != Notes->end(); i++)
	{
		if (i->Measure == measure)
			retval.push_back(*i);
	}
	return retval;
}