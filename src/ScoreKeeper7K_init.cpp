#include "Global.h"
#include "ScoreKeeper7K.h"

void ScoreKeeper7K::init(){
	
	use_w0 = false; // don't use Ridiculous by default.
	use_w0_for_ex2 = false;
	
	rank_pts = 0;
	use_bbased = false;
	avg_hit = 0;
	
	pacemaker_texts[PMT_F] = "F" ;
	pacemaker_texts[PMT_E] = "E" ;
	pacemaker_texts[PMT_D] = "D" ;
	pacemaker_texts[PMT_C] = "C" ;
	pacemaker_texts[PMT_B] = "B" ;
	pacemaker_texts[PMT_A] = "A" ;
	pacemaker_texts[PMT_AA] = "AA" ;
	pacemaker_texts[PMT_AAA] = "AAA" ;

	pacemaker_texts[PMT_RANK_ZERO] = "0" ;
	pacemaker_texts[PMT_RANK_P1] = "+1" ;
	pacemaker_texts[PMT_RANK_P2] = "+2" ;
	pacemaker_texts[PMT_RANK_P3] = "+3" ;
	pacemaker_texts[PMT_RANK_P4] = "+4" ;
	pacemaker_texts[PMT_RANK_P5] = "+5" ;
	pacemaker_texts[PMT_RANK_P6] = "+6" ;
	pacemaker_texts[PMT_RANK_P7] = "+7" ;
	pacemaker_texts[PMT_RANK_P8] = "+8" ;
	pacemaker_texts[PMT_RANK_P9] = "+9" ;

	setAccMin(6.4);
	setAccMax(100);

	max_notes = 0;

	score = 0;

	rank_w0_count = 0;
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
	exp_hit_score = 0;
	exp_score = 0;
	exp3_score = 0;

	sc_score = 0;
	sc_sc_score = 0;

	combo = 0;
	max_combo = 0;

	coolcombo = 0;
	pills = 0;
	jams = 0;
	jam_jchain = 0;
	o2_score = 0;

	total_sqdev = 0;
	accuracy = 0;

	lifebar_groove = 0.20;
	lifebar_easy = 0.20;
	lifebar_survival = 1;
	lifebar_exhard = 1;
	lifebar_death = 1;

	lifebar_stepmania = 0.50;
	
	setO2LifebarRating(2); // HX by default.

	double inc[6] = {+0.010, +0.008, +0.004, 0, -0.04, -0.08};
	setLifeIncrements(inc, 6);
	setMissDecrement(0.08);
	setEarlyMissDecrement(0.02);

	judge_window_scale = 1.00;
	set_timing_windows();

}

void ScoreKeeper7K::set_beat_timing_windows()
{
	// This in beats.
	double o2jamTimingAmt[] =
	{
		0.664 * 0.2, // COOL threshold
		0.664 * 0.5, // GOOD threshold
		0.664 * 0.8, //  BAD threshold
		0.664 // MISS threshold
	};

	judgment_time[SKJ_W1] = o2jamTimingAmt[0];
	judgment_time[SKJ_W2] = o2jamTimingAmt[1];
	judgment_time[SKJ_W3] = o2jamTimingAmt[2];
	
	// No early misses, only plain misses.
	earlymiss_threshold = miss_threshold = o2jamTimingAmt[3];
}

void ScoreKeeper7K::set_timing_windows(){
	
	double JudgmentValues[] = { 6.4, 16, 40, 100, 250, 625 };

	miss_threshold = 250;
	earlymiss_threshold = 1250;

	for (auto i = 0; i < sizeof(JudgmentValues)/sizeof(double); i++)
		judgment_time[i] = JudgmentValues[i] * judge_window_scale;

	for (auto i = 0; i < 9; i++)
		judgment_amt[i] = 0;

	for (auto i = -127; i < 128; ++i)
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

void ScoreKeeper7K::setSMJ4Windows()
{
	// Ridiculous is included: J7 Marvelous.
	double JudgmentValues[] = { 11.25, 22.5, 45, 90, 135, 180 };

	miss_threshold = 180;
	earlymiss_threshold = 180;
	for (int i = 0; i < sizeof(JudgmentValues) / sizeof(double); i++)
		judgment_time[i] = JudgmentValues[i];

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

	if (rank == -100) // We assume we're dealing with beats-based timing.
	{
		use_bbased = true;
		use_w0 = false;
		set_beat_timing_windows();
		return;
	}

	use_bbased = false;
	switch(rank){
		case 0:
			judge_window_scale = 0.50; break;
		case 1:
			judge_window_scale = 0.75; break;
		case 2:
			judge_window_scale = 1.00; break;
		case 3:
			judge_window_scale = 1.50; break;
		case 4:
			judge_window_scale = 2.00; break;
	}
	set_timing_windows();
}


void ScoreKeeper7K::setJudgeScale(double scale){
	judge_window_scale = scale;
	set_timing_windows();
}
