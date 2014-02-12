#include "ScoreKeeper.h"

#define CLAMP(var, min, max) (var) < (min) ? (min) : (var) > (max) ? (max) : (var)
#define max(a, b) (a) > (b) ? (a) : (b)
#define min(a, b) (a) < (b) ? (a) : (b)

ScoreKeeper7K::~ScoreKeeper7K(){ ; }

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
	// recalculate groove lifebar increments.
	lifebar_easy_increment = CLAMP(5.0 / max_notes, 0.002, 0.8);
	lifebar_groove_increment = CLAMP(3.0 / max_notes, 0.001, 0.8);
	lifebar_survival_increment = 1.2 / max_notes;
	lifebar_exhard_increment = 0.5 / max_notes;
}

void ScoreKeeper7K::setEX2(int ms){ EX2 = ms; if(EX2 > EX1) EX1 = EX2 + 1; }
void ScoreKeeper7K::setEX1(int ms){ EX1 = ms; if(EX1 < EX2) EX2 = EX1 - 1; }

void ScoreKeeper7K::setDP2(int ms){ DP2 = ms; if(DP2 > DP1) DP1 = DP2 + 1; }
void ScoreKeeper7K::setDP1(int ms){ DP1 = ms; if(DP1 < DP2) DP2 = DP1 - 1; }

void ScoreKeeper7K::hitNote(int ms){
	
	// interesting stuff goes here.

// hit notes

	++total_notes;
	++notes_hit;

// combo

	if(ms < ACC_MAX){
		++combo;
		if(combo > max_combo)
			max_combo = combo;
	}
	
// EX Score

	ex_score += ms <= EX2 ? 2 : ms <= EX1 ? 1 : 0;

// Numerical score / money score

	sc_score += CLAMP(accuracy_percent(ms * ms) / 100, 0, 1) * 2;
	sc_sc_score += sc_score * CLAMP(accuracy_percent(ms * ms) / 100, 0, 1);
	
	score = float(SCORE_MAX * sc_sc_score) / (max_notes * (max_notes + 1)); 

// accuracy score

	total_sqdev += ms * ms;
	accuracy = accuracy_percent(total_sqdev / total_notes);

// lifebars
	
	if(ms < ACC_MAX){
		lifebar_groove = min(1, lifebar_groove + lifebar_groove_increment);
		if(lifebar_survival > 0)
			lifebar_survival = min(1, lifebar_survival + lifebar_survival_increment);
		if(lifebar_exhard > 0)
			lifebar_exhard = min(1, lifebar_exhard + lifebar_exhard_increment);
		lifebar_easy = min(1, lifebar_easy + lifebar_easy_increment);
	}else{
		// miss tier 1
		lifebar_groove = max(0, lifebar_groove - 0.02);
		lifebar_survival = max(0, lifebar_survival - CLAMP(50.0 / max_notes, 0.02, 0.20));
		lifebar_exhard = max(0, lifebar_survival - CLAMP(100.0 / max_notes, 0.10, 0.50));
		lifebar_easy = max(0, lifebar_groove - 0.02);
	}

}


void ScoreKeeper7K::missNote(bool auto_hold_miss){
	
	total_sqdev += ACC_CUTOFF * ACC_CUTOFF;

	++total_notes;
	accuracy = accuracy_percent(total_sqdev / total_notes);
	
	combo = 0;

	// miss tier 2
	lifebar_groove = max(0, lifebar_groove - 0.06);
	lifebar_survival = max(0, lifebar_survival - CLAMP(100.0 / max_notes, 0.06, 0.50));
	lifebar_exhard = max(0, lifebar_survival - CLAMP(200.0 / max_notes, 0.20, 0.80));
	lifebar_easy = max(0, lifebar_groove - 0.06);

}

double ScoreKeeper7K::getAccCutoff(){
	return ACC_CUTOFF;
}

double ScoreKeeper7K::getAccMax()
{
	return ACC_MAX;
}

/* actual score functions. */

int ScoreKeeper7K::getScore(ScoreType score_type){

	switch(score_type){
		case ST_SCORE:
			return int(score);
		case ST_EX:
			return ex_score;
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
		case PST_EX:
			return float(ex_score) / float(total_notes) * 100.0;
		case PST_ACC:
			return accuracy;
		default:
			return 0;
	}

}

int ScoreKeeper7K::getLifebarUnits(LifeType lifebar_unit_type){
	
}

float ScoreKeeper7K::getLifebarAmount(LifeType lifebar_amount_type){
	
	switch(lifebar_amount_type){
		case LT_GROOVE:
			return lifebar_groove;
		case LT_SURVIVaL:
			return lifebar_survival;
		default:
			return 0;
	}

}

