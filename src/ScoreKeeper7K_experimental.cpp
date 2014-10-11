#include "Global.h"
#include "ScoreKeeper7K.h"


/*

EXP^2 system

The way this system works is similar to how combo scoring works in Beatmania.

The max score for this system is 1,200,000 points, which is a normalized version of another score.

The score that a note receives is based on your current "point combo".

- W3 increases the point combo by  5.
- W2 increases the point combo by  9.
- W1 increases the point combo by 10.

This point combo starts at the beginning of a song and has a maximum bound of 100.
However, the point combo resets to a maximum of 90 after a note is judged, so a W3 cannot get more than 95 points, and a W2 cannot get more than 99.

Also, the point combo resets to 0 if a note is missed.

*/

void ScoreKeeper7K::update_exp2(int ms){

	if(ms <= judgment_time[SKJ_W1]){
		exp_combo += 4;
	}else if(ms <= judgment_time[SKJ_W2]){
		exp_combo += 3;
	}else if(ms <= judgment_time[SKJ_W3]){
		exp_combo += 2;
	}else{
		exp_combo = 0;
	}

	exp_combo_pts += exp_combo;

	if(exp_combo > 96) exp_combo = 96;

	exp_score = 1.2e6 * exp_combo_pts / exp_max_combo_pts;

}
