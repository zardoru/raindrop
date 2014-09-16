#include "Global.h"
#include "ScoreKeeper.h"

void ScoreKeeper7K::update_ranks(int ms){

	if (ms < judgement_time[SKJ_W1]) ++rank_w1_count;
	if (ms < judgement_time[SKJ_W2]) ++rank_w2_count;
	if (ms < judgement_time[SKJ_W3]) ++rank_w3_count;

	int rank_w1_pts = max(rank_w1_count * 2 - total_notes, 0);
	int rank_w2_pts = max(rank_w2_count * 2 - total_notes, 0);
	int rank_w3_pts = max(rank_w3_count * 2 - total_notes, 0);

	rank_pts = rank_w1_pts + rank_w2_pts + rank_w3_pts;

}

int ScoreKeeper7K::getRank(){

	// find some way to streamline this.

	if(rank_pts == total_notes * 300 / 100) return 10;
	if(rank_pts >= total_notes * 280 / 100) return 9;
	if(rank_pts >= total_notes * 260 / 100) return 8;
	if(rank_pts >= total_notes * 240 / 100) return 7;
	if(rank_pts >= total_notes * 220 / 100) return 6;
	if(rank_pts >= total_notes * 200 / 100) return 5;
	if(rank_pts >= total_notes * 180 / 100) return 4;
	if(rank_pts >= total_notes * 160 / 100) return 3;
	if(rank_pts >= total_notes * 140 / 100) return 2;
	if(rank_pts >= total_notes * 120 / 100) return 1;
	if(rank_pts >= total_notes * 100 / 100) return 0;
	if(rank_pts >= total_notes * 90 / 100) return -1;
	if(rank_pts >= total_notes * 80 / 100) return -2;
	if(rank_pts >= total_notes * 70 / 100) return -3;
	if(rank_pts >= total_notes * 60 / 100) return -4;
	if(rank_pts >= total_notes * 50 / 100) return -5;
	if(rank_pts >= total_notes * 40 / 100) return -6;
	if(rank_pts >= total_notes * 20 / 100) return -7;
	if(rank_pts > 0) return -8;
	return -9;

}
