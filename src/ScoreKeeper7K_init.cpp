#include "Global.h"
#include "ScoreKeeper7K.h"

void ScoreKeeper7K::init(){
	
	use_w0 = false; // don't use Ridiculous by default.
	use_w0_for_ex2 = false;
	
	pacemaker_texts[PMT_F] = "F" ;
	pacemaker_texts[PMT_E] = "E" ;
	pacemaker_texts[PMT_D] = "D" ;
	pacemaker_texts[PMT_C] = "C" ;
	pacemaker_texts[PMT_B] = "B" ;
	pacemaker_texts[PMT_A] = "A" ;
	pacemaker_texts[PMT_AA] = "AA" ;
	pacemaker_texts[PMT_AAA] = "AAA" ;

	setAccMin(6.4);
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
	
	osu_points = 0;
	bonus_counter = 100;
	osu_bonus_points = 0;

	osu_score = 0;
	osu_accuracy = 100;
	
	exp_combo = 0;
	exp_combo_pts = 0;
	exp_max_combo_pts = 0;

	sc_score = 0;
	sc_sc_score = 0;

	combo = 0;
	max_combo = 0;

	total_sqdev = 0;
	accuracy = 0;

	lifebar_groove = 0.20;
	lifebar_easy = 0.20;
	lifebar_survival = 1;
	lifebar_exhard = 1;
	lifebar_death = 1;

	lifebar_stepmania = 0.50;
	
	double inc[6] = {+0.010, +0.008, +0.004, 0, -0.04, -0.08};
	setLifeIncrements(inc, 5);
	setMissDecrement(0.08);
	setEarlyMissDecrement(0.02);

	judge_window_scale = 1.00;
	set_timing_windows();

}


void ScoreKeeper7K::set_timing_windows(){
	
	double JudgmentValues[] = { 6.4, 16, 40, 100, 250, 625 };

	miss_threshold = 250;
	earlymiss_threshold = 1250;

	for (int i = 0; i < sizeof(JudgmentValues)/sizeof(double); i++)
		judgment_time[i] = JudgmentValues[i] * judge_window_scale;

	for (int i = 0; i < 9; i++)
		judgment_amt[i] = 0;

	for (int i = -127; i < 128; ++i)
		histogram[i+127] = 0;

}


void ScoreKeeper7K::setODWindows(int od){

	use_w0 = true; // if chart has OD, use osu!mania scoring.
	use_w0_for_ex2 = true;

	double JudgmentValues[] = { 16, 34, 67, 97, 121, 158 };

	miss_threshold = 121 + (10 - od) * 3;
	earlymiss_threshold = 158 + (10 - od) * 3;
	
	judgment_time[SKJ_W0] = JudgmentValues[SKJ_W0];
	for (int i = 1; i < sizeof(JudgmentValues)/sizeof(double); i++)
		judgment_time[i] = JudgmentValues[i] + (10 - od) * 3;

	for (int i = 0; i < 9; i++)
		judgment_amt[i] = 0;

	for (int i = -127; i < 128; ++i)
		histogram[i+127] = 0;

}


void ScoreKeeper7K::set_manual_w0(bool on){ use_w0 = on; } // make a config option


ScoreKeeper7K::ScoreKeeper7K(){
	init();
}

ScoreKeeper7K::ScoreKeeper7K(double judge_window_scale){
	init();
	this->judge_window_scale = judge_window_scale;
	set_timing_windows();
}
