#include "pch.h"


#include "ScoreKeeper7K.h"

void ScoreKeeper7K::update_bms(ScoreKeeperJudgment judgment){

	if(!use_w0_for_ex2 && judgment <= SKJ_W1 || use_w0_for_ex2 && judgment == SKJ_W0){
		ex_score += 2;
		bms_dance_pts += 15;
	}else if(!use_w0_for_ex2 && judgment == SKJ_W2 || use_w0_for_ex2 && judgment == SKJ_W1){
		ex_score += 1;
		bms_dance_pts += 10;
	}else if(!use_w0_for_ex2 && judgment == SKJ_W3 || use_w0_for_ex2 && judgment == SKJ_W2){
		bms_dance_pts += 2;
	}else{
		bms_combo = -1;
	}

	bms_combo = min(10LL, bms_combo + 1);
	bms_combo_pts += bms_combo;

	bms_score =
		  50000 * bms_combo_pts / bms_max_combo_pts
		+ 10000 * bms_dance_pts / max_notes;

}

void ScoreKeeper7K::update_lr2(ScoreKeeperJudgment judgment){

	if(!use_w0_for_ex2 && judgment <= SKJ_W1 || use_w0_for_ex2 && judgment == SKJ_W0){
		lr2_dance_pts += 10;
	}else if(!use_w0_for_ex2 && judgment == SKJ_W2 || use_w0_for_ex2 && judgment == SKJ_W1){
		lr2_dance_pts += 5;
	}else if(!use_w0_for_ex2 && judgment == SKJ_W3 || use_w0_for_ex2 && judgment == SKJ_W2){
		lr2_dance_pts += 1;
	}

	lr2_score = 20000 * lr2_dance_pts / max_notes;

}

int ScoreKeeper7K::getBMRank()
{
	double thresholds[] = { 8.0 / 9.0, 7.0 / 9.0, 6.0 / 9.0, 
		5.0 / 9.0, 4.0 / 9.0, 3.0 / 9.0, 2.0 / 9.0, 1.0 / 9.0, 
		0, -std::numeric_limits<double>::infinity() };

	double exps = getPercentScore(PST_EX);
	auto rank_index = 9;

	for (auto i = 0; i < sizeof(thresholds) / sizeof(double); i++)
	{
		if (exps > thresholds[i] * 100)
		{
			rank_index = i;
			break;
		}
	}

	switch (rank_index)
	{
	case 0: return PMT_AAA;
	case 1: return PMT_AA;
	case 2: return PMT_A;
	case 3: return PMT_B;
	case 4: return PMT_C;
	case 5: return PMT_D;
	case 6: return PMT_E;
	case 7: 
	default:
		return PMT_F;
	}
}


