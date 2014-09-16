#include "Global.h"
#include "ScoreKeeper.h"

void ScoreKeeper7K::update_bms(int ms, bool hit){

	if(hit){

		if(ms <= judgement_time[SKJ_W1]){ // make times based on judgments.
			ex_score += 2;
			bms_dance_pts += 15;
		}else if(ms <= judgement_time[SKJ_W2]){
			ex_score += 1;
			bms_dance_pts += 10;
		}else{
			bms_dance_pts += 2;
		}

		bms_combo = min(10, bms_combo + 1);
		bms_combo_pts += bms_combo;

		bms_score =
			  50000 * bms_combo_pts / bms_max_combo_pts
			+ 10000 * bms_dance_pts / max_notes;

	}else{

		bms_combo = 0;

	}

}

void ScoreKeeper7K::update_lr2(int ms, bool hit){

	

}
