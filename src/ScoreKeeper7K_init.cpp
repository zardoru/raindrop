#include "Global.h"
#include "ScoreKeeper7K.h"

ScoreKeeper7K::ScoreKeeper7K(){

	setAccMin(16);
	setAccMax(100);

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

	lr2_dance_pts = 0;
	lr2_score = 0;

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

	judge_window_scale = 1.00;
	// double JudgmentValues[] = { 16, 40, 100, 250, 625 };
	double JudgmentValues[] = { 16, 40, 100, 250, 625 };
	
	miss_threshold = 250;
	earlymiss_threshold = 1250;

	for (int i = 0; i < sizeof(JudgmentValues)/sizeof(double); i++)
		judgment_time[i+1] = JudgmentValues[i] * judge_window_scale;

	for (int i = 0; i < 9; i++)
		judgment_amt[i] = 0;

	for (int i = -127; i < 128; ++i)
		histogram[i+127] = 0;

}
