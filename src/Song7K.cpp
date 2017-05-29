#include "pch.h"

#include "GameGlobal.h"
#include "Configuration.h"
#include "Logging.h"
#include "Song7K.h"


using namespace Game::VSRG;

ChartType ChartInfo::GetType() const
{
    return Type;
}

Song::Song()
{
    Mode = MODE_VSRG;
}

Song::~Song()
{
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
