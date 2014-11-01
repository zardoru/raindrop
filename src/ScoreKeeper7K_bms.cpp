#include "Global.h"
#include "ScoreKeeper7K.h"

void ScoreKeeper7K::update_bms(int ms, bool hit){

	if(hit){

		if(ms <= judgment_time[SKJ_W1]){ // make times based on judgments.a
			if(use_w0 && use_w0_for_ex2 && ms > judgment_time[SKJ_W0]){
				ex_score += 1;	
			}else{
				ex_score += 2;
			}
			bms_dance_pts += 15;
		}else if(ms <= judgment_time[SKJ_W2]){
			if(!(use_w0 && use_w0_for_ex2)){
				ex_score += 1;
			}
			bms_dance_pts += 10;
		}else{
			bms_dance_pts += 2;
		}

		bms_combo = min(10LL, bms_combo + 1);
		bms_combo_pts += bms_combo;

		bms_score =
			  50000 * bms_combo_pts / bms_max_combo_pts
			+ 10000 * bms_dance_pts / max_notes;

	}else{

		bms_combo = 0;

	}

}

void ScoreKeeper7K::update_lr2(int ms, bool hit){

	if(hit){

		if(ms <= judgment_time[SKJ_W1]){ // make times based on judgments.
			lr2_dance_pts += 10;
		}else if(ms <= judgment_time[SKJ_W2]){
			lr2_dance_pts += 5;
		}else{
			lr2_dance_pts += 1;
		}

		lr2_score = 20000 * lr2_dance_pts / max_notes;

	}

}


