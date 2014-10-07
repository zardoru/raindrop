#include "Global.h"
#include "ScoreKeeper7K.h"

#include <iomanip>

ScoreKeeper7K::~ScoreKeeper7K(){  }

double ScoreKeeper7K::accuracy_percent(float var){
	return float(ACC_MAX_SQ - var) / (ACC_MAX_SQ - ACC_MIN_SQ) * 100;
}

void ScoreKeeper7K::setAccMin(int ms){
	ACC_MIN = ms;
	ACC_MIN_SQ = ms * ms;
}

void ScoreKeeper7K::setAccMax(int ms){
	ACC_MAX = ms;
	ACC_MAX_SQ = ms * ms;
}

void ScoreKeeper7K::setMaxNotes(int notes){

	max_notes = notes;

	if(notes < 10) bms_max_combo_pts = notes * (notes + 1) / 2;
	else bms_max_combo_pts = 55 + (notes - 10) * 10;

	// recalculate groove lifebar increments.
	lifebar_easy_increment = Clamp(5.0 / max_notes, 0.002, 0.8);
	lifebar_groove_increment = Clamp(3.0 / max_notes, 0.001, 0.8);
	lifebar_survival_increment = 1.2 / max_notes;
	lifebar_exhard_increment = 0.5 / max_notes;

}

int ScoreKeeper7K::getMaxNotes(){ return max_notes; }

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


// Numerical score / money score

	sc_score += Clamp(accuracy_percent(ms * ms) / 100, 0.0, 1.0) * 2;
	sc_sc_score += sc_score * Clamp(accuracy_percent(ms * ms) / 100, 0.0, 1.0);

	score = double(SCORE_MAX * sc_sc_score) / (max_notes * (max_notes + 1));

// accuracy score

	total_sqdev += ms * ms;
	accuracy = accuracy_percent(total_sqdev / total_notes);

// lifebars

	if(ms < judgment_time[SKJ_W3]){
		lifebar_groove = min(1.0, lifebar_groove + lifebar_groove_increment);
		if(lifebar_survival > 0)
			lifebar_survival = min(1.0, lifebar_survival + lifebar_survival_increment);
		if(lifebar_exhard > 0)
			lifebar_exhard = min(1.0, lifebar_exhard + lifebar_exhard_increment);
		lifebar_easy = min(1.0, lifebar_easy + lifebar_easy_increment);
	}else{
		// miss tier 1
		lifebar_groove = max(0.0, lifebar_groove - 0.02);
		lifebar_survival = max(0.0, lifebar_survival - Clamp(50.0 / max_notes, 0.02, 0.20));
		lifebar_exhard = max(0.0, lifebar_survival - Clamp(100.0 / max_notes, 0.10, 0.50));
		lifebar_easy = max(0.0, lifebar_groove - 0.02);
	}

// judgments

	ScoreKeeperJudgment judgment = SKJ_NONE;

	for (int i = 1; i < 6; i++)
	{
		if (ms <= judgment_time[i])
		{
			judgment_amt[(ScoreKeeperJudgment)i]++;
			judgment = ScoreKeeperJudgment((ScoreKeeperJudgment)i);
			break;
		}
	}

// Other methods

	update_ranks(ms); // rank calculation

	update_bms(ms, ms <= judgment_time[SKJ_W3]);
	update_lr2(ms, ms <= judgment_time[SKJ_W3]);

	return judgment;

}

int ScoreKeeper7K::getJudgmentCount(ScoreKeeperJudgment judgment)
{
	return judgment_amt[judgment];
}


void ScoreKeeper7K::missNote(bool auto_hold_miss, bool early_miss){

	judgment_amt[SKJ_MISS]++;

	if(!early_miss)
		++total_notes;
	
	accuracy = accuracy_percent(total_sqdev / total_notes);

	if(!auto_hold_miss && !early_miss){

		total_sqdev += getMissCutoff() * getMissCutoff();
		combo = 0;

		// miss tier 2
		lifebar_groove = max(0.0, lifebar_groove - 0.06);
		lifebar_survival = max(0.0, lifebar_survival - Clamp(100.0 / max_notes, 0.06, 0.50));
		lifebar_exhard = max(0.0, lifebar_survival - Clamp(200.0 / max_notes, 0.20, 0.80));
		lifebar_easy = max(0.0, lifebar_easy - 0.06);

	}else if(early_miss){

		// miss tier 1
		lifebar_groove = max(0.0, lifebar_groove - 0.02);
		lifebar_survival = max(0.0, lifebar_survival - Clamp(50.0 / max_notes, 0.02, 0.20));
		lifebar_exhard = max(0.0, lifebar_survival - Clamp(100.0 / max_notes, 0.10, 0.50));
		lifebar_easy = max(0.0, lifebar_groove - 0.02);

	}

	// other methods
	update_bms(1000, false);

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

int ScoreKeeper7K::getJudgmentWindow(ScoreKeeperJudgment judgment){
	return judgment_time[judgment];
}


std::string ScoreKeeper7K::getHistogram(){

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

int ScoreKeeper7K::getScore(ScoreType score_type){

	switch(score_type){
		case ST_SCORE:
			return int(score);
		case ST_EX:
			return ex_score;
		case ST_IIDX:
			return bms_score;
		case ST_LR2:
			return lr2_score;
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

float ScoreKeeper7K::getPercentScore(PercentScoreType percent_score_type){

	switch(percent_score_type){
		case PST_RANK:
			return float(rank_pts) / float(total_notes) * 100.0;
		case PST_EX:
			return float(ex_score) / float(total_notes * 2) * 100.0;
		case PST_ACC:
			return accuracy;
		case PST_NH:
			return float(notes_hit) / float(total_notes) * 100.0;
		default:
			return 0;
	}

}

int ScoreKeeper7K::getLifebarUnits(LifeType lifebar_unit_type){
		return 0;
}

float ScoreKeeper7K::getLifebarAmount(LifeType lifebar_amount_type){

	switch(lifebar_amount_type){
		case LT_GROOVE:
			return lifebar_groove;
		case LT_SURVIVAL:
			return lifebar_survival;
		default:
			return 0;
	}

}
