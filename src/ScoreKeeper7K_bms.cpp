#include "Global.h"
#include "ScoreKeeper7K.h"

void ScoreKeeper7K::update_bms(ScoreKeeperJudgment judgment){

	if(!use_w0_for_ex2 && judgment == SKJ_W1 || use_w0_for_ex2 && judgment == SKJ_W0){
		ex_score += 2;
		bms_dance_pts += 15;
	}else if(!use_w0_for_ex2 && judgment == SKJ_W2 || use_w0_for_ex2 && judgment == SKJ_W1){
		ex_score += 1;
		bms_dance_pts += 10;
	}else if(!use_w0_for_ex2 && judgment == SKJ_W3 || use_w0_for_ex2 && judgment == SKJ_W2){
		bms_dance_pts += 2;
	}else{
		bms_combo = -1;
	}

	bms_combo = min(10LL, bms_combo + 1);
	bms_combo_pts += bms_combo;

	bms_score =
		  50000 * bms_combo_pts / bms_max_combo_pts
		+ 10000 * bms_dance_pts / max_notes;

}

void ScoreKeeper7K::update_lr2(ScoreKeeperJudgment judgment){

	if(!use_w0_for_ex2 && judgment == SKJ_W1 || use_w0_for_ex2 && judgment == SKJ_W0){
		lr2_dance_pts += 10;
	}else if(!use_w0_for_ex2 && judgment == SKJ_W2 || use_w0_for_ex2 && judgment == SKJ_W1){
		lr2_dance_pts += 5;
	}else if(!use_w0_for_ex2 && judgment == SKJ_W3 || use_w0_for_ex2 && judgment == SKJ_W2){
		lr2_dance_pts += 1;
	}

	lr2_score = 20000 * lr2_dance_pts / max_notes;

}


