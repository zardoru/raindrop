#include "Global.h"
#include "ScoreKeeper7K.h"


void ScoreKeeper7K::setO2LifebarRating(int difficulty)
{
	difficulty = Clamp(difficulty, 0, 2);

	lifebar_o2jam = 1;

	// Thanks to Entozer for giving this approximate information.
	switch (difficulty)
	{
	case 0: // EX
		lifebar_o2jam_increment = (19.0 / 20.0) / 290.0;
		lifebar_o2jam_decrement = 1.0 / 20.0;
		break;
	case 1: // NX
		lifebar_o2jam_increment = (24.0 / 25.0) / 470.0;
		lifebar_o2jam_decrement = 1.0 / 25.0;
		break;
	case 2: // HX
		lifebar_o2jam_increment = (33.0 / 34.0) / 950.0;
		lifebar_o2jam_decrement = 1.0 / 34.0;
		break;
	}
}

bool ScoreKeeper7K::usesO2()
{
	return use_bbased;
}

int ScoreKeeper7K::getCoolCombo()
{
	return coolcombo;
}

uint8 ScoreKeeper7K::getPills()
{
	return pills;
}

