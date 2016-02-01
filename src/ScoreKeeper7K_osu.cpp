#include "pch.h"

#include "Global.h"
#include "ScoreKeeper7K.h"

void ScoreKeeper7K::update_osu(ScoreKeeperJudgment judgment){
	
	int osu_bonus_multiplier = 0;

	switch(judgment){
		case SKJ_W0:
			osu_points += 320;
			osu_accuracy += 300;
			osu_bonus_multiplier = 32;
			bonus_counter = min(100, bonus_counter + 2);
			break;
		case SKJ_W1:
			osu_points += 300;
			osu_accuracy += 300;
			osu_bonus_multiplier = 32;
			bonus_counter = min(100, bonus_counter + 1);
			break;
		case SKJ_W2:
			osu_points += 200;
			osu_accuracy += 200;
			osu_bonus_multiplier = 16;
			bonus_counter = max(0, bonus_counter - 8);
			break;
		case SKJ_W3:
			osu_points += 100;
			osu_accuracy += 100;
			osu_bonus_multiplier = 8;
			bonus_counter = max(0, bonus_counter - 24);
			break;
		case SKJ_W4:
			osu_points += 50;
			osu_accuracy += 50;
			osu_bonus_multiplier = 4;
			bonus_counter = max(0, bonus_counter - 44);
			break;
		case SKJ_MISS:
			osu_points += 0;
			bonus_counter = 0;
			break;
	}

	osu_bonus_points += osu_bonus_multiplier * sqrt(double(bonus_counter));
	
	osu_score = 500000 * ((osu_points + osu_bonus_points) / double(max_notes * 320));

}
