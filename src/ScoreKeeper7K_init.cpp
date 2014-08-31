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
	
	rank_w1_count = 0;
	rank_w2_count = 0;
	rank_w3_count = 0;
	
	notes_hit = 0;
	total_notes = 0;

	ex_score = 0;
	
	bms_combo = 0;
	bms_combo_pts = 0;
	bms_dance_pts = 0;
	bms_score = 0;

	sc_score = 0;
	sc_sc_score = 0;

	combo = 0;
	max_combo = 0;

	total_sqdev = 0;
	accuracy = 0;

	lifebar_groove = 0.78;
	lifebar_survival = 1;
	lifebar_exhard = 1;
	lifebar_death = 1;
	lifebar_easy = 0.78;

	double JudgementValues[] = { 22.5, 45, 90, 135, 180 };

	for (int i = 0; i < sizeof(JudgementValues)/sizeof(double); i++)
		judgement_time[i+1] = JudgementValues[i];

	for (int i = 0; i < 9; i++)
		judgement_amt[i] = 0;
}

