#include "Global.h"
#include "ScoreKeeper7K.h"


/*

EXP^2 system

The way this system works is similar to how combo scoring works in Beatmania.

The max score for this system is 1,200,000 points, which is a normalized version of another score.

The score that a note receives is based on your current "point combo".

- W3 increases the point combo by  2.
- W2 increases the point combo by  3.
- W1 increases the point combo by  4.

This point combo starts at the beginning of a song and has a maximum bound of 100.
However, the point combo resets to a maximum of 96 after a note is judged, so a W3 cannot get more than 98 points, and a W2 cannot get more than 99.

Also, the point combo resets to 0 if a note is missed.

*/

void ScoreKeeper7K::update_exp2(ScoreKeeperJudgment judgment){

	if(!use_w0_for_ex2 && judgment == SKJ_W1 || use_w0_for_ex2 && judgment == SKJ_W0){
		exp_combo += 4;
	}else if(!use_w0_for_ex2 && judgment == SKJ_W2 || use_w0_for_ex2 && judgment == SKJ_W1){
		exp_combo += 3;
	}else if(!use_w0_for_ex2 && judgment == SKJ_W3 || use_w0_for_ex2 && judgment == SKJ_W2){
		exp_combo += 2;
	}else if(use_w0_for_ex2 && judgment == SKJ_W3){
		exp_combo = 1;
	}else{
		exp_combo = 0;
	}

	exp_combo_pts += exp_combo;

	if(exp_combo > 96) exp_combo = 96;

	exp_score = 1.2e6 * exp_combo_pts / exp_max_combo_pts;

}
