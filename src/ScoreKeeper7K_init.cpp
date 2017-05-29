#include "pch.h"

#include "ScoreKeeper7K.h"

namespace Game {
	namespace VSRG {

		void ScoreKeeper::init()
		{
			use_w0 = false; // don't use Ridiculous by default.
			use_w0_for_ex2 = false;

			rank_pts = 0;
			use_o2jam = false;
			avg_hit = 0;
			hit_variance = 0;

			pacemaker_texts[PMT_F] = "F";
			pacemaker_texts[PMT_E] = "E";
			pacemaker_texts[PMT_D] = "D";
			pacemaker_texts[PMT_C] = "C";
			pacemaker_texts[PMT_B] = "B";
			pacemaker_texts[PMT_A] = "A";
			pacemaker_texts[PMT_AA] = "AA";
			pacemaker_texts[PMT_AAA] = "AAA";

			pacemaker_texts[PMT_RANK_ZERO] = "0";
			pacemaker_texts[PMT_RANK_P1] = "+1";
			pacemaker_texts[PMT_RANK_P2] = "+2";
			pacemaker_texts[PMT_RANK_P3] = "+3";
			pacemaker_texts[PMT_RANK_P4] = "+4";
			pacemaker_texts[PMT_RANK_P5] = "+5";
			pacemaker_texts[PMT_RANK_P6] = "+6";
			pacemaker_texts[PMT_RANK_P7] = "+7";
			pacemaker_texts[PMT_RANK_P8] = "+8";
			pacemaker_texts[PMT_RANK_P9] = "+9";

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

			double inc[6] = { +0.010, +0.008, +0.004, 0, -0.04, -0.08 };
			setLifeIncrements(inc, 6);
			setMissDecrement(0.08);
			setEarlyMissDecrement(0.02);

			memset(judgment_time, 0, sizeof(judgment_time));
			judge_window_scale = 1.00;
			setBMSTimingWindows();
		}

		void ScoreKeeper::setO2JamBeatTimingWindows()
		{
			// This in beats.
			double o2jamTimingAmt[] =
			{
				O2_WINDOW * 0.1, // raindrop!o2jam extension: XCOOL
				O2_WINDOW * 0.2, // COOL threshold
				O2_WINDOW * 0.5, // GOOD threshold
				O2_WINDOW * 0.8, //  BAD threshold
				O2_WINDOW // MISS threshold
			};

			judgment_time[SKJ_W0] = o2jamTimingAmt[0];
			judgment_time[SKJ_W1] = o2jamTimingAmt[1];
			judgment_time[SKJ_W2] = o2jamTimingAmt[2];
			judgment_time[SKJ_W3] = o2jamTimingAmt[3];

			// No early misses, only plain misses.
			earlymiss_threshold = miss_threshold = o2jamTimingAmt[4];
		}

		void ScoreKeeper::setBMSTimingWindows()
		{
			// double JudgmentValues[] = { 6.4, 16, 40, 100, 250, -1, 625 };
			double JudgmentValues[] = { 8.8, 22, 55, 137.5, 250, -1, 625 };

			miss_threshold = 250;
			earlymiss_threshold = 625;

			for (auto i = 0; i < sizeof(JudgmentValues) / sizeof(double); i++)
				judgment_time[i] = JudgmentValues[i] * judge_window_scale;

			for (auto i = 0; i < sizeof(judgment_amt) / sizeof(double); i++)
				judgment_amt[i] = 0;

			for (auto i = -127; i < 128; ++i)
				histogram[i + 127] = 0;
		}

		void ScoreKeeper::setODWindows(int od)
		{
			use_w0 = true; // if chart has OD, use osu!mania scoring.
			use_w0_for_ex2 = true;
			
			/*
			double JudgmentValues[] = { 16, 34, 67, 97, 121, -1, 158 };

			miss_threshold = 121 + (10 - od) * 3;
			// earlymiss_threshold = 158 + (10 - od) * 3;

			judgment_time[SKJ_W0] = JudgmentValues[SKJ_W0];
			for (int i = 1; i < sizeof(JudgmentValues) / sizeof(double); i++)
				judgment_time[i] = JudgmentValues[i] + (10 - od) * 3;

			*/

			double JudgmentValues[] = { 16, 64, 97, 127, 151, -1, 188 };

			miss_threshold = 188 - od * 3;
			earlymiss_threshold = miss_threshold;

			judgment_time[SKJ_W0] = JudgmentValues[SKJ_W0];
			for (int i = 1; i < sizeof(JudgmentValues) / sizeof(double); i++)
				judgment_time[i] = JudgmentValues[i] - od * 3;

			for (int i = 0; i < 9; i++)
				judgment_amt[i] = 0;

			for (int i = -127; i < 128; ++i)
				histogram[i + 127] = 0;
		}

		void ScoreKeeper::setSMJ4Windows()
		{
			// Ridiculous is included: J7 Marvelous.
			// No early miss threshold
			double JudgmentValues[] = { 11.25, 22.5, 45, 90, 135, 180, 180 };

			miss_threshold = 180;
			earlymiss_threshold = 180;
			for (int i = 0; i < sizeof(JudgmentValues) / sizeof(double); i++)
				judgment_time[i] = JudgmentValues[i];
		}

		void ScoreKeeper::setUseW0(bool on) { use_w0 = on; } // make a config option

		ScoreKeeper::ScoreKeeper()
		{
			init();
		}

		ScoreKeeper::ScoreKeeper(double judge_window_scale)
		{
			init();
			this->judge_window_scale = judge_window_scale;
			setBMSTimingWindows();
		}

		void ScoreKeeper::setLifeTotal(double total, double multiplier)
		{
			double effmul = isnan(multiplier) ? 1 : multiplier;

			if (total != -1 && isnan(multiplier) && !isnan(total)) lifebar_total = total;
			else lifebar_total = std::max(260.0, 7.605 * max_notes / (6.5 + 0.01 * max_notes)) * effmul;

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

		void ScoreKeeper::setLifeIncrements(double* increments, int inc_n)
		{
			for (int a = 0; a < inc_n; ++a)
			{
				life_increment[a] = increments[a];
			}
		}

		void ScoreKeeper::setMissDecrement(double decrement)
		{
			lifebar_stepmania_miss_decrement = decrement;
		}

		void ScoreKeeper::setEarlyMissDecrement(double decrement)
		{
			lifebar_stepmania_earlymiss_decrement = decrement;
		}

		void ScoreKeeper::setJudgeRank(int rank) {

			if (rank == -100) // We assume we're dealing with beats-based timing.
			{
				use_o2jam = true;
				use_w0 = false;
				setO2JamBeatTimingWindows();
				return;
			}

			use_o2jam = false;

			// old values: 0.5, 0.75, 1.0, 1.5, 2.0
			switch (rank) {
			case 0:
				judge_window_scale = 8.0 / 11.0; break;
			case 1:
				judge_window_scale = 9.0 / 11.0; break;
			case 2:
				judge_window_scale = 1.00; break;
			case 3:
				judge_window_scale = 13.0 / 11.0; break;
			case 4:
				judge_window_scale = 16.0 / 11.0; break;
			}
			setBMSTimingWindows();
		}

		void ScoreKeeper::setJudgeScale(double scale)
		{
			judge_window_scale = scale;
			setBMSTimingWindows();
		}
	}
}