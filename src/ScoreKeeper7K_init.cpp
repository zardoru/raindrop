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

	lifebar_groove = 0.2;
	lifebar_survival = 1;
	lifebar_exhard = 1;
	lifebar_death = 1;
	lifebar_easy = 0.2;

}

