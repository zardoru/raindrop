#include "ScoreKeeper.h"

ScoreKeeper7K::ScoreKeeper7K(){
	
	setAccMin(16);
	setAccMax(100);
	ACC_CUTOFF = 135;

	setEX2(20);
	setEX1(40);

	setDP2(45);
	setDP1(90);

	max_notes = 0;

	score = 0;

	notes_hit = 0;
	total_notes = 0;

	ex_score = 0;
	sc_score = 0;
	sc_sc_score = 0;

	combo = 0;
	max_combo = 0;

	total_sqdev = 0;
	accuracy = 0;

}

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
}

void ScoreKeeper7K::setEX2(int ms){ EX2 = ms; if(EX2 > EX1) EX1 = EX2 + 1; }
void ScoreKeeper7K::setEX1(int ms){ EX1 = ms; if(EX1 < EX2) EX2 = EX1 - 1; }

void ScoreKeeper7K::setDP2(int ms){ DP2 = ms; if(DP2 > DP1) DP1 = DP2 + 1; }
void ScoreKeeper7K::setDP1(int ms){ DP1 = ms; if(DP1 < DP2) DP2 = DP1 - 1; }


#define CLAMP(var, min, max) (var) < (min) ? (min) : (var) > (max) ? (max) : (var)


void ScoreKeeper7K::hitNote(int ms){
	
	total_sqdev += ms * ms;

	++total_notes;
	++notes_hit;

	if(ms > ACC_MAX){
		++combo;
		if(combo > max_combo)
			max_combo = combo;
	}
	
	ex_score += ms <= EX2 ? 2 : ms <= EX1 ? 1 : 0;

	sc_score += CLAMP(accuracy_percent(ms * ms) / 100, 0, 1) * 2;
	sc_sc_score += sc_score * CLAMP(accuracy_percent(ms * ms) / 100, 0, 1);
	
	score = float(SCORE_MAX * sc_sc_score) / (max_notes * (max_notes + 1)); 

	accuracy = accuracy_percent(total_sqdev / total_notes);

}


void ScoreKeeper7K::missNote(bool auto_hold_miss){
	
	total_sqdev += ACC_CUTOFF * ACC_CUTOFF;

	++total_notes;
	accuracy = accuracy_percent(total_sqdev / total_notes);
	
	combo = 0;

}

int ScoreKeeper7K::getAccCutoff(){
	return ACC_CUTOFF;
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
