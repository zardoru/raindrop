#include "rmath.h"
#include <game/Song.h>


using namespace rd;

ChartType ChartInfo::GetType() const
{
    return Type;
}

Difficulty* Song::GetDifficulty(uint32_t i)
{
    if (i >= Difficulties.size())
        return nullptr;
    else
        return Difficulties.at(i).get();
}

uint8_t Song::GetDifficultyCount()
{
	return Difficulties.size();
}

void Difficulty::Destroy()
{
    if (Data)
        Data = nullptr;

    Timing.clear(); Timing.shrink_to_fit();
    Author.clear(); Author.shrink_to_fit();
	Filename = "";
}


uint32_t rd::DifficultyLoadInfo::GetObjectCount()
{
	uint32_t cnt = 0;
	for (auto measure : Measures) {
		for (auto i = 0; i < MAX_CHANNELS; i++) {
			for (auto note : measure.Notes[i]) {
				cnt++;
			}
		}
	}

	return cnt;
}

uint32_t rd::DifficultyLoadInfo::GetScoreItemsCount()
{
	uint32_t cnt = 0;
	for (auto measure : Measures) {
		for (auto & Note : measure.Notes) {
			for (auto note : Note) {
				if (note.NoteKind == NK_FAKE ||
					note.NoteKind == NK_INVISIBLE ||
					note.NoteKind == NK_MINE)
					continue;

				if (note.EndTime != 0 && 
					note.NoteKind == NK_NORMAL)
					cnt += 2;
				else
					cnt++;
			}
		}
	}

	return cnt;
}
