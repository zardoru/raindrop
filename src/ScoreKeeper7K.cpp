#include "Global.h"
#include "ScoreKeeper7K.h"

#include <iomanip>
#include <map>

ScoreKeeper7K::~ScoreKeeper7K(){  }

double ScoreKeeper7K::accuracy_percent(double var){
	return double(ACC_MAX_SQ - var) / (ACC_MAX_SQ - ACC_MIN_SQ) * 100;
}

void ScoreKeeper7K::setAccMin(double ms){
	ACC_MIN = ms;
	ACC_MIN_SQ = ms * ms;
}

void ScoreKeeper7K::setAccMax(double ms){
	ACC_MAX = ms;
	ACC_MAX_SQ = ms * ms;
}


void ScoreKeeper7K::setMaxNotes(int notes){

	max_notes = notes;

	if(notes < 10) bms_max_combo_pts = notes * (notes + 1) / 2;
	else bms_max_combo_pts = 55 + (notes - 10) * 10;

	if(notes < 25) exp_max_combo_pts = notes * (notes + 1) * 2;
	else exp_max_combo_pts = 1300 + (notes - 25) * 100;

}

int ScoreKeeper7K::getMaxNotes(){ return max_notes; }


void ScoreKeeper7K::setLifeTotal(double total){

	if(total != -1) lifebar_total = total;
	else lifebar_total = max(260.0, 7.605 * max_notes / (6.5 + 0.01 * max_notes));

	// recalculate groove lifebar increments.
	lifebar_easy_increment = Clamp(lifebar_total / max_notes / 50.0, 0.004, 0.8);
	lifebar_groove_increment = Clamp(lifebar_total / max_notes / 100.0, 0.002, 0.8);
	lifebar_survival_increment = lifebar_total / max_notes / 200.0;
	lifebar_exhard_increment = lifebar_total / max_notes / 200.0;

	lifebar_easy_decrement = Clamp(lifebar_total / max_notes / 12.0, 0.00, 0.02);
	lifebar_groove_decrement = Clamp(lifebar_total / max_notes / 10.0, 0.01, 0.02);
	lifebar_survival_decrement = Clamp(lifebar_total / max_notes / 7.0, 0.02, 0.15);
	lifebar_exhard_decrement = Clamp(lifebar_total / max_notes / 3.0, 0.03, 0.3);

}


void ScoreKeeper7K::setLifeIncrements(double* increments, int inc_n){
	for (int a = 0; a < inc_n; ++a){
		life_increment[a] = increments[a];
	}
}

void ScoreKeeper7K::setMissDecrement(double decrement){
	lifebar_stepmania_miss_decrement = decrement;
}

void ScoreKeeper7K::setEarlyMissDecrement(double decrement){
	lifebar_stepmania_earlymiss_decrement = decrement;
}


void ScoreKeeper7K::setJudgeRank(int rank){
	switch(rank){
		case 0:
			judge_window_scale = 0.50; break;
		case 1:
			judge_window_scale = 0.75; break;
		case 2:
			judge_window_scale = 1.00; break;
		case 3:
			judge_window_scale = 1.50; break;
	}
	set_timing_windows();
}


void ScoreKeeper7K::setJudgeScale(double scale){
	judge_window_scale = scale;
	set_timing_windows();
}



int ScoreKeeper7K::getTotalNotes(){ return total_notes; }


ScoreKeeperJudgment ScoreKeeper7K::hitNote(int ms){

	// interesting stuff goes here.

// hit notes

	++total_notes;

	if(abs(ms) < 128)
	++histogram[ms + 127];

	ms = abs(ms);

// combo

	if(ms <= judgment_time[SKJ_W3]){
		++notes_hit;
		++combo;
		if(combo > max_combo)
			max_combo = combo;
	}else{
		combo = 0;
	}



// accuracy score

	total_sqdev += ms * ms;
	accuracy = accuracy_percent(total_sqdev / total_notes);



// judgments

	ScoreKeeperJudgment judgment = SKJ_NONE;

	for (int i = (use_w0 ? 0 : 1); i < 6; i++)
	{
		if (ms <= judgment_time[i])
		{
			judgment_amt[(ScoreKeeperJudgment)i]++;
			judgment = ScoreKeeperJudgment((ScoreKeeperJudgment)i);
			break;
		}
	}



// SC, ACC^2 score

	sc_score += Clamp(accuracy_percent(ms * ms) / 100, 0.0, 1.0) * 2;
	sc_sc_score += sc_score * Clamp(accuracy_percent(ms * ms) / 100, 0.0, 1.0);

	score = double(SCORE_MAX * sc_sc_score) / (max_notes * (max_notes + 1));



// lifebars

	if(ms <= judgment_time[SKJ_W3]){

		lifebar_easy = min(1.0, lifebar_easy + lifebar_easy_increment);
		lifebar_groove = min(1.0, lifebar_groove + lifebar_groove_increment);
		if(lifebar_survival > 0)
			lifebar_survival = min(1.0, lifebar_survival + lifebar_survival_increment);
		if(lifebar_exhard > 0)
			lifebar_exhard = min(1.0, lifebar_exhard + lifebar_exhard_increment);

	}else{

		// miss tier 1
		lifebar_easy = max(0.0, lifebar_easy - lifebar_easy_decrement);
		lifebar_groove = max(0.0, lifebar_groove - lifebar_groove_decrement);
		lifebar_survival = max(0.0, lifebar_survival - lifebar_groove_decrement);
		lifebar_exhard = max(0.0, lifebar_exhard - lifebar_exhard_decrement);
		
		lifebar_death = 0;

	}
	
	lifebar_stepmania = min(1.0, lifebar_stepmania + life_increment[judgment]);


// Other methods

	update_ranks(ms); // rank calculation

	update_bms(ms, ms <= judgment_time[SKJ_W3]); // Beatmania scoring
	update_lr2(ms, ms <= judgment_time[SKJ_W3]); // Lunatic Rave 2 scoring

	update_exp2(ms);

	update_osu(ms, judgment);

	return judgment;

}

int ScoreKeeper7K::getJudgmentCount(int judgment)
{
	if (judgment >= 9 || judgment < 0) return 0;

	return judgment_amt[judgment];
}

bool ScoreKeeper7K::usesW0(){ return use_w0; }

void ScoreKeeper7K::missNote(bool auto_hold_miss, bool early_miss){

	judgment_amt[SKJ_MISS]++;

	if(!early_miss)
		++total_notes;

	accuracy = accuracy_percent(total_sqdev / total_notes);

	if(!auto_hold_miss && !early_miss){

		total_sqdev += getMissCutoff() * getMissCutoff();
		combo = 0;

		// miss tier 2
		lifebar_easy = max(0.0, lifebar_easy - lifebar_easy_decrement * 3);
		lifebar_groove = max(0.0, lifebar_groove - lifebar_groove_decrement * 3);
		lifebar_survival = max(0.0, lifebar_survival - lifebar_survival_decrement * 3);
		lifebar_exhard = max(0.0, lifebar_exhard - lifebar_exhard_decrement * 3);

		lifebar_death = 0;

		lifebar_stepmania = max(0.0, lifebar_stepmania - lifebar_stepmania_miss_decrement);

	}else if(early_miss){

		// miss tier 1
		lifebar_easy = max(0.0, lifebar_easy - lifebar_easy_decrement);
		lifebar_groove = max(0.0, lifebar_groove - lifebar_groove_decrement);
		lifebar_survival = max(0.0, lifebar_survival - lifebar_survival_decrement);
		lifebar_exhard = max(0.0, lifebar_exhard - lifebar_exhard_decrement);

		lifebar_stepmania = max(0.0, lifebar_stepmania - lifebar_stepmania_earlymiss_decrement);

	}

	// other methods
	update_bms(1000, false);
	update_exp2(1000);
	update_osu(1000, SKJ_MISS);

}

double ScoreKeeper7K::getEarlyMissCutoff(){
	return earlymiss_threshold;
}

double ScoreKeeper7K::getMissCutoff(){
	return miss_threshold;
}

double ScoreKeeper7K::getAccMax(){
	return ACC_MAX;
}

double ScoreKeeper7K::getJudgmentWindow(int judgment){
	if (judgment >= 9 || judgment < 0) return 0;
	return judgment_time[judgment];
}


GString ScoreKeeper7K::getHistogram(){

	std::stringstream ss;

	for (int i = 0; i < 255; ++i){
		int it = (i % 5) * 51 + (i / 5); // transpose
	 	ss << std::setw(4) << it - 127 << ": " << std::setw(4) << histogram[it] << " ";
		if(i % 5 == 4)
			ss << "\n";
	}

	return ss.str();

}



/* actual score functions. */

int ScoreKeeper7K::getScore(int score_type){

	switch(score_type){
		case ST_SCORE:
			return int(score);
		case ST_EX:
			return ex_score;
		case ST_IIDX:
			return bms_score;
		case ST_LR2:
			return lr2_score;
		case ST_EXP:
			return exp_score;
		case ST_OSUMANIA:
			return osu_score;
		case ST_COMBO:
			return combo;
		case ST_MAX_COMBO:
			return max_combo;
		case ST_NOTES_HIT:
			return notes_hit;
		default:
			return 0;
	}

}

float ScoreKeeper7K::getPercentScore(int percent_score_type){

	switch(percent_score_type){
		case PST_RANK:
			return double(rank_pts) / double(total_notes) * 100.0;
		case PST_EX:
			return double(ex_score) / double(total_notes * 2) * 100.0;
		case PST_ACC:
			return accuracy;
		case PST_NH:
			return double(notes_hit) / double(total_notes) * 100.0;
		case PST_OSU:
			return double(osu_accuracy) / double(total_notes) / 3.0;
		default:
			return 0;
	}

}

int ScoreKeeper7K::getLifebarUnits(int lifebar_unit_type){

	return 0;

}

float ScoreKeeper7K::getLifebarAmount(int lifebar_amount_type){

	switch(lifebar_amount_type){
		case LT_EASY:
			return lifebar_easy;
		case LT_GROOVE:
			return lifebar_groove;
		case LT_SURVIVAL:
			return lifebar_survival;
		case LT_EXHARD:
			return lifebar_exhard;
		case LT_DEATH:
			return lifebar_death;
		case LT_STEPMANIA:
			return lifebar_stepmania;
		default:
			return 0;
	}

}

bool ScoreKeeper7K::isStageFailed(int lifebar_amount_type){
	
	switch(lifebar_amount_type){
		case LT_GROOVE:
			return total_notes == max_notes && lifebar_easy < 0.80;
		case LT_EASY:
			return total_notes == max_notes && lifebar_groove < 0.80;
		case LT_SURVIVAL:
			return lifebar_survival == 0.0;
		case LT_EXHARD:
			return lifebar_exhard == 0.0;
		case LT_DEATH:
			return lifebar_death == 0.0;
		case LT_STEPMANIA:
			return lifebar_stepmania == 0.0;
		default:
			return false;
	}

}


void ScoreKeeper7K::failStage(){
	
	total_notes = max_notes;
	
	update_ranks(1000); // rank calculation
	update_bms(1000, false); // Beatmania scoring
	update_lr2(1000, false); // Lunatic Rave 2 scoring
	update_exp2(1000);
	update_osu(1000, SKJ_MISS);
	
}


int ScoreKeeper7K::getPacemakerDiff(PacemakerType pacemaker){
	
	switch(pacemaker){
		case PMT_F:
			return ex_score - (total_notes * 2 / 9 + (total_notes * 2 % 9 != 0)); 
		case PMT_E:
			return ex_score - (total_notes * 4 / 9 + (total_notes * 4 % 9 != 0)); 
		case PMT_D:
			return ex_score - (total_notes * 6 / 9 + (total_notes * 6 % 9 != 0)); 
		case PMT_C:
			return ex_score - (total_notes * 8 / 9 + (total_notes * 8 % 9 != 0)); 
		case PMT_B:
			return ex_score - (total_notes * 10 / 9 + (total_notes * 10 % 9 != 0)); 
		case PMT_A:
			return ex_score - (total_notes * 12 / 9 + (total_notes * 12 % 9 != 0)); 
		case PMT_AA:
			return ex_score - (total_notes * 14 / 9 + (total_notes * 14 % 9 != 0)); 
		case PMT_AAA:
			return ex_score - (total_notes * 16 / 9 + (total_notes * 16 % 9 != 0));

		case PMT_50EX:
			return ex_score - (total_notes); 
		case PMT_75EX:
			return ex_score - (total_notes * 75 / 50); 
		case PMT_85EX:
			return ex_score - (total_notes * 85 / 50);

		case PMT_RANK_P1:
			return rank_pts - (total_notes * 120 / 100);
		case PMT_RANK_P2:
			return rank_pts - (total_notes * 140 / 100);
		case PMT_RANK_P3:
			return rank_pts - (total_notes * 160 / 100);
		case PMT_RANK_P4:
			return rank_pts - (total_notes * 180 / 100);
		case PMT_RANK_P5:
			return rank_pts - (total_notes * 200 / 100);
		case PMT_RANK_P6:
			return rank_pts - (total_notes * 220 / 100);
		case PMT_RANK_P7:
			return rank_pts - (total_notes * 240 / 100);
		case PMT_RANK_P8:
			return rank_pts - (total_notes * 260 / 100);
		case PMT_RANK_P9:
			return rank_pts - (total_notes * 280 / 100);
	}

	return 0;
}





std::pair<GString, int> ScoreKeeper7K::getAutoPacemaker(){
	
	PacemakerType pmt;
	
	if (ex_score < total_notes * 2 / 9)  pmt = PMT_F;
	else if (ex_score < total_notes * 5 / 9)  pmt = PMT_E;
	else if (ex_score < total_notes * 7 / 9)  pmt = PMT_D;
	else if (ex_score < total_notes * 9 / 9)  pmt = PMT_C;
	else if (ex_score < total_notes * 11 / 9)  pmt = PMT_B;
	else if (ex_score < total_notes * 13 / 9)  pmt = PMT_A;
	else if (ex_score < total_notes * 15 / 9)  pmt = PMT_AA;
	else  pmt = PMT_AAA;

	int pacemaker = getPacemakerDiff(pmt);
	std::stringstream ss;
	ss
	<< std::setfill(' ') << std::setw(4) << pacemaker_texts[pmt] << ": ";

	return std::make_pair(ss.str(), pacemaker);

}
