#include "pch.h"

#include "ScoreKeeper7K.h"

namespace Game {
	namespace VSRG {
		ScoreKeeper::~ScoreKeeper() {}

		double ScoreKeeper::accuracy_percent(double var)
		{
			return double(ACC_MAX_SQ - var) / (ACC_MAX_SQ - ACC_MIN_SQ) * 100;
		}

		void ScoreKeeper::setAccMin(double ms)
		{
			ACC_MIN = ms;
			ACC_MIN_SQ = ms * ms;
		}

		void ScoreKeeper::setAccMax(double ms)
		{
			ACC_MAX = ms;
			ACC_MAX_SQ = ms * ms;
		}

		void ScoreKeeper::setMaxNotes(int notes)
		{
			max_notes = std::max(notes, 1);

			if (notes < 10) bms_max_combo_pts = notes * (notes + 1) / 2;
			else bms_max_combo_pts = 55 + (notes - 10) * 10;

			if (notes < 25) exp_max_combo_pts = notes * (notes + 1) * 2;
			else exp_max_combo_pts = 1300 + (notes - 25) * 100;
		}

		int ScoreKeeper::getMaxNotes() const 
		{ return max_notes; }

		int ScoreKeeper::getTotalNotes() const
		{ return total_notes; }


		float ScoreKeeper::getHitStDev() const
		{
			return sqrt(hit_variance / (total_notes - 1));
		}

		// ms is misleading- since it may very well be beats, but it's fine.
		ScoreKeeperJudgment ScoreKeeper::hitNote(double ms)
		{
			// hit notes

			// online variance and average hit
			++total_notes;
			float delta = ms - avg_hit;
			avg_hit += delta / total_notes;

			hit_variance += delta * (ms - avg_hit);

			

			// std::cerr << use_bbased << " " << ms << " ";

			if (use_o2jam)
			{
				auto dist = ms / O2_WINDOW * 128;
				if (abs(dist) < 128)
				{
					++histogram[static_cast<int>(round(dist)) + 127];
				}
			}
			else
			{
				if (abs(ms) < 128)
				{
					++histogram[static_cast<int>(round(ms)) + 127];
				}
			}

			ms = abs(ms);

			// combo
			auto increase_combo = [&]() {
				++notes_hit;
				++combo;
				if (combo > max_combo)
					max_combo = combo;
			};

			// check combo for o2jam later (after transformed judgment)
			double combo_leniency;
			if (!use_o2jam)
			{
				combo_leniency = judgment_time[SKJ_W3]; // GOODs/GREATs or less

				if (ms <= combo_leniency)
				{
					increase_combo();
				}
				else
				{
					combo = 0;
				}
			}

			// accuracy score

			if (use_o2jam)
				total_sqdev += ms * ms / pow(O2_WINDOW, 2);
			else
				total_sqdev += ms * ms;

			accuracy = accuracy_percent(total_sqdev / total_notes);

			// judgments

			ScoreKeeperJudgment judgment = SKJ_NONE;

			for (int i = (use_w0 ? 0 : 1); i < (use_o2jam ? 4 : 6); i++)
			{
				if (ms <= judgment_time[i])
				{
					judgment = ScoreKeeperJudgment(i);

					if (!use_o2jam)
						judgment_amt[judgment]++;
					else
					{
						// we using o2 based mechanics..
						auto j = getO2Judge(judgment);

						// transform this judgment if neccesary (pills/etc)
						judgment = ScoreKeeperJudgment(j);
						judgment_amt[j]++;
					}

					// breaking early is important.
					break;
				}
			}

			// since it may be transformed we check for combo here instead
			if (use_o2jam) {
				if (judgment < SKJ_W3) {
					increase_combo();
				}
				else {
					combo = 0;
				}
			}

			// SC, ACC^2 score

			sc_score += Clamp(accuracy_percent(ms * ms) / 100, 0.0, 1.0) * 2;
			sc_sc_score += sc_score * Clamp(accuracy_percent(ms * ms) / 100, 0.0, 1.0);

			score = double(SCORE_MAX * sc_sc_score) / (max_notes * (max_notes + 1));

			// lifebars

			lifebarHit(ms, judgment);

			if (judgment == SKJ_NONE)
				std::cerr << "Error, invalid judgment: " << ms << "\n";

			// std::cerr << std::endl;

			// Other methods

			update_ranks(judgment); // rank calculation
			update_bms(judgment); // Beatmania scoring
			update_lr2(judgment); // Lunatic Rave 2 scoring
			update_exp2(judgment);
			update_osu(judgment);
			update_o2(judgment);

			return judgment;
		}

		void ScoreKeeper::lifebarHit(double ms, Game::VSRG::ScoreKeeperJudgment judgment)
		{
			if (ms <= judgment_time[SKJ_W3])
			{
				lifebar_easy = std::min(1.0, lifebar_easy + lifebar_easy_increment);
				lifebar_groove = std::min(1.0, lifebar_groove + lifebar_groove_increment);

				if (lifebar_survival > 0)
					lifebar_survival = std::min(1.0, lifebar_survival + lifebar_survival_increment);
				if (lifebar_exhard > 0)
					lifebar_exhard = std::min(1.0, lifebar_exhard + lifebar_exhard_increment);
			}
			else
			{
				// miss tier 1
				lifebar_easy = std::max(0.0, lifebar_easy - lifebar_easy_decrement);
				lifebar_groove = std::max(0.0, lifebar_groove - lifebar_groove_decrement);
				lifebar_survival = std::max(0.0, lifebar_survival - lifebar_groove_decrement);
				lifebar_exhard = std::max(0.0, lifebar_exhard - lifebar_exhard_decrement);

				lifebar_death = 0;
			}

			if (ms <= judgment_time[SKJ_W1])
			{ // only COOLs restore o2jam lifebar
				lifebar_o2jam = std::min(1.0, lifebar_o2jam + lifebar_o2jam_increment);
			}
			else if (ms > judgment_time[SKJ_W2]) // BADs get some HP from you,
				lifebar_o2jam = std::max(0.0, lifebar_o2jam - lifebar_o2jam_decrement_bad);

			// std::cerr << ms << " " << judgment << " " << life_increment[judgment] << std::endl;

			lifebar_stepmania = std::min(1.0, lifebar_stepmania + life_increment[judgment]);
		}

		int ScoreKeeper::getJudgmentCount(int judgment)
		{
			if (judgment >= 9 || judgment < 0) return 0;

			return judgment_amt[judgment];
		}

		bool ScoreKeeper::usesW0() const { 
			return use_w0; 
		}

		void ScoreKeeper::missNote(bool dont_break_combo, bool early_miss)
		{
			judgment_amt[SKJ_MISS]++;

			if (!early_miss)
				++total_notes;

			accuracy = accuracy_percent(total_sqdev / total_notes);

			if (!early_miss)
			{
				if (!dont_break_combo) {
					total_sqdev += getMissCutoffMS() * getMissCutoffMS();
					combo = 0;
				}

				// miss tier 2
				lifebar_easy = std::max(0.0, lifebar_easy - lifebar_easy_decrement * 3);
				lifebar_groove = std::max(0.0, lifebar_groove - lifebar_groove_decrement * 3);
				lifebar_survival = std::max(0.0, lifebar_survival - lifebar_survival_decrement * 3);
				lifebar_exhard = std::max(0.0, lifebar_exhard - lifebar_exhard_decrement * 3);

				lifebar_death = 0;

				lifebar_stepmania = std::max(0.0, lifebar_stepmania - lifebar_stepmania_miss_decrement);

				lifebar_o2jam = std::max(0.0, lifebar_o2jam - lifebar_o2jam_decrement);
			}
			else if (early_miss)
			{
				// miss tier 1
				lifebar_easy = std::max(0.0, lifebar_easy - lifebar_easy_decrement);
				lifebar_groove = std::max(0.0, lifebar_groove - lifebar_groove_decrement);
				lifebar_survival = std::max(0.0, lifebar_survival - lifebar_survival_decrement);
				lifebar_exhard = std::max(0.0, lifebar_exhard - lifebar_exhard_decrement);

				lifebar_stepmania = std::max(0.0, lifebar_stepmania - lifebar_stepmania_earlymiss_decrement);
			}

			// other methods
			update_bms(SKJ_MISS);
			update_exp2(SKJ_MISS);
			update_osu(SKJ_MISS);
			update_o2(SKJ_MISS);
		}

		double ScoreKeeper::getJudgmentCutoff() {
			auto rt = 0.0;
			rt = std::max(std::max(miss_threshold, rt), earlymiss_threshold);
			return rt;
		}

		double ScoreKeeper::getEarlyMissCutoffMS() const
		{
			return earlymiss_threshold;
		}

		double ScoreKeeper::getMissCutoffMS() const
		{
			return miss_threshold;
		}

		double ScoreKeeper::getAccMax() const
		{
			return ACC_MAX;
		}

		double ScoreKeeper::getJudgmentWindow(int judgment)
		{
			if (judgment >= 9 || judgment < 0) return 0;
			return judgment_time[judgment];
		}

		std::string ScoreKeeper::getHistogram()
		{
			std::stringstream ss;

			const int HISTOGRAM_DISPLAY_WIDTH = 15;

			for (int i = 0; i < 255; ++i)
			{
				int it = (i % HISTOGRAM_DISPLAY_WIDTH) * (255 / HISTOGRAM_DISPLAY_WIDTH) + (i / HISTOGRAM_DISPLAY_WIDTH); // transpose
				ss << std::setw(4) << it - 127 << ": " << std::setw(4) << histogram[it] << " ";
				if (i % HISTOGRAM_DISPLAY_WIDTH == HISTOGRAM_DISPLAY_WIDTH - 1)
					ss << "\n";
			}

			return ss.str();
		}

		int ScoreKeeper::getHistogramPoint(int point) const
		{
			int msCount = sizeof(histogram) / sizeof(double) / 2;
			if (abs(point) > msCount) return 0;
			return histogram[point + msCount];
		}

		int ScoreKeeper::getHistogramPointCount() const
		{
			return sizeof(histogram) / sizeof(double);
		}

		int ScoreKeeper::getHistogramHighestPoint() const
		{
			return std::accumulate(&histogram[0], histogram + getHistogramPointCount(), 1.0, [](double a, double b) -> double
			{
				return std::max(a, b);
			});
		}

		double ScoreKeeper::getAvgHit() const
		{
			return avg_hit;
		}

		/* actual score functions. */

		int ScoreKeeper::getScore(int score_type)
		{
			switch (score_type)
			{
			case ST_SCORE:
				return int(score);
			case ST_EX:
				return ex_score;
			case ST_IIDX:
				return bms_score;
			case ST_LR2:
				return lr2_score;
			case ST_EXP:
				return exp_score;
			case ST_EXP3:
				return exp3_score;
			case ST_OSUMANIA:
				return osu_score;
			case ST_COMBO:
				return combo;
			case ST_MAX_COMBO:
				return max_combo;
			case ST_NOTES_HIT:
				return notes_hit;
			case ST_O2JAM:
				return o2_score;
			default:
				return 0;
			}
		}

		float ScoreKeeper::getPercentScore(int percent_score_type) const {
			switch (percent_score_type) {
			case PST_RANK:
				if (total_notes)
					return double(rank_pts) / double(total_notes) * 100.0;
				return 100;
			case PST_EX:
				if (total_notes)
					return double(ex_score) / double(total_notes * 2) * 100.0;
				return 100;
			case PST_ACC:
				return accuracy;
			case PST_NH:
				if (total_notes)
					return double(notes_hit) / double(total_notes) * 100.0;
				return 100;
			case PST_OSU:
				if (total_notes)
					return double(osu_accuracy) / double(total_notes) / 3.0;
				return 100;
			default:
				return 0;
			}
		}

		int ScoreKeeper::getLifebarUnits(int lifebar_unit_type)
		{
			return 0;
		}

		float ScoreKeeper::getLifebarAmount(int lifebar_amount_type)
		{
			switch (lifebar_amount_type)
			{
			case LT_EASY:
				return lifebar_easy;
			case LT_GROOVE:
				return lifebar_groove;
			case LT_SURVIVAL:
				return lifebar_survival;
			case LT_EXHARD:
				return lifebar_exhard;
			case LT_DEATH:
				return lifebar_death;
			case LT_STEPMANIA:
				return lifebar_stepmania;
			case LT_O2JAM:
				return lifebar_o2jam;
			default:
				return 0;
			}
		}

		bool ScoreKeeper::isStageFailed(int lifebar_amount_type)
		{
			switch (lifebar_amount_type)
			{
			case LT_GROOVE:
				return total_notes == max_notes && lifebar_groove < 0.80;
			case LT_EASY:
				return total_notes == max_notes && lifebar_easy < 0.80;
			case LT_SURVIVAL:
				return lifebar_survival <= 0.0;
			case LT_EXHARD:
				return lifebar_exhard <= 0.0;
			case LT_DEATH:
				return lifebar_death <= 0.0;
			case LT_STEPMANIA:
				return lifebar_stepmania <= 0.0;
			case LT_O2JAM:
				return lifebar_o2jam <= 0.0;
			default:
				return false;
			}
		}

		bool ScoreKeeper::hasDelayedFailure(int lifebar_type)
		{
			switch (lifebar_type)
			{
			case LT_GROOVE:
			case LT_EASY:
				return true;
			default:
				return false;
			}
		}

		void ScoreKeeper::failStage()
		{
			total_notes = max_notes;

			update_ranks(SKJ_MISS); // rank calculation
			update_bms(SKJ_MISS); // Beatmania scoring
			update_lr2(SKJ_MISS); // Lunatic Rave 2 scoring
			update_exp2(SKJ_MISS);
			update_osu(SKJ_MISS);
		}

		int ScoreKeeper::getPacemakerDiff(PacemakerType pacemaker)
		{
			switch (pacemaker)
			{
			case PMT_F:
				return ex_score - (total_notes * 2 / 9 + (total_notes * 2 % 9 != 0));
			case PMT_E:
				return ex_score - (total_notes * 4 / 9 + (total_notes * 4 % 9 != 0));
			case PMT_D:
				return ex_score - (total_notes * 6 / 9 + (total_notes * 6 % 9 != 0));
			case PMT_C:
				return ex_score - (total_notes * 8 / 9 + (total_notes * 8 % 9 != 0));
			case PMT_B:
				return ex_score - (total_notes * 10 / 9 + (total_notes * 10 % 9 != 0));
			case PMT_A:
				return ex_score - (total_notes * 12 / 9 + (total_notes * 12 % 9 != 0));
			case PMT_AA:
				return ex_score - (total_notes * 14 / 9 + (total_notes * 14 % 9 != 0));
			case PMT_AAA:
				return ex_score - (total_notes * 16 / 9 + (total_notes * 16 % 9 != 0));

			case PMT_50EX:
				return ex_score - (total_notes);
			case PMT_75EX:
				return ex_score - (total_notes * 75 / 50);
			case PMT_85EX:
				return ex_score - (total_notes * 85 / 50);

			case PMT_RANK_ZERO:
				return rank_pts - (total_notes * 100 / 100);
			case PMT_RANK_P1:
				return rank_pts - (total_notes * 120 / 100 + (total_notes * 120 % 100 != 0));
			case PMT_RANK_P2:
				return rank_pts - (total_notes * 140 / 100 + (total_notes * 140 % 100 != 0));
			case PMT_RANK_P3:
				return rank_pts - (total_notes * 160 / 100 + (total_notes * 160 % 100 != 0));
			case PMT_RANK_P4:
				return rank_pts - (total_notes * 180 / 100 + (total_notes * 180 % 100 != 0));
			case PMT_RANK_P5:
				return rank_pts - (total_notes * 200 / 100);
			case PMT_RANK_P6:
				return rank_pts - (total_notes * 220 / 100 + (total_notes * 220 % 100 != 0));
			case PMT_RANK_P7:
				return rank_pts - (total_notes * 240 / 100 + (total_notes * 240 % 100 != 0));
			case PMT_RANK_P8:
				return rank_pts - (total_notes * 260 / 100 + (total_notes * 260 % 100 != 0));
			case PMT_RANK_P9:
				return rank_pts - (total_notes * 280 / 100 + (total_notes * 280 % 100 != 0));

			default:
				break;
			}

			return 0;
		}

		std::pair<std::string, int> ScoreKeeper::getAutoPacemaker()
		{
			PacemakerType pmt;

			if (ex_score < total_notes * 2 / 9)  pmt = PMT_F;
			else if (ex_score < total_notes * 5 / 9)  pmt = PMT_E;
			else if (ex_score < total_notes * 7 / 9)  pmt = PMT_D;
			else if (ex_score < total_notes * 9 / 9)  pmt = PMT_C;
			else if (ex_score < total_notes * 11 / 9)  pmt = PMT_B;
			else if (ex_score < total_notes * 13 / 9)  pmt = PMT_A;
			else if (ex_score < total_notes * 15 / 9)  pmt = PMT_AA;
			else  pmt = PMT_AAA;

			int pacemaker = getPacemakerDiff(pmt);
			std::stringstream ss;
			ss
				<< std::setfill(' ') << std::setw(4) << pacemaker_texts[pmt] << ": ";

			return std::make_pair(ss.str(), pacemaker);
		}

		std::pair<std::string, int> ScoreKeeper::getAutoRankPacemaker()
		{
			PacemakerType pmt;
			if (rank_pts < total_notes * 110 / 100)  pmt = PMT_RANK_ZERO;
			else if (rank_pts < total_notes * 130 / 100)  pmt = PMT_RANK_P1;
			else if (rank_pts < total_notes * 150 / 100)  pmt = PMT_RANK_P2;
			else if (rank_pts < total_notes * 170 / 100)  pmt = PMT_RANK_P3;
			else if (rank_pts < total_notes * 190 / 100)  pmt = PMT_RANK_P4;
			else if (rank_pts < total_notes * 210 / 100)  pmt = PMT_RANK_P5;
			else if (rank_pts < total_notes * 230 / 100)  pmt = PMT_RANK_P6;
			else if (rank_pts < total_notes * 250 / 100)  pmt = PMT_RANK_P7;
			else if (rank_pts < total_notes * 270 / 100)  pmt = PMT_RANK_P8;
			else  pmt = PMT_RANK_P9;

			int pacemaker = getPacemakerDiff(pmt);

			std::stringstream ss;
			ss
				<< std::setfill(' ') << std::setw(4) << pacemaker_texts[pmt] << ": ";

			return std::make_pair(ss.str(), pacemaker);
		}

		void ScoreKeeper::update_bms(ScoreKeeperJudgment judgment)
		{
			if ((!use_w0_for_ex2 && judgment <= SKJ_W1) || (use_w0_for_ex2 && judgment == SKJ_W0))
			{
				ex_score += 2;
				bms_dance_pts += 15;
			}
			else if ((!use_w0_for_ex2 && judgment == SKJ_W2) || (use_w0_for_ex2 && judgment == SKJ_W1))
			{
				ex_score += 1;
				bms_dance_pts += 10;
			}
			else if ((!use_w0_for_ex2 && judgment == SKJ_W3) || (use_w0_for_ex2 && judgment == SKJ_W2))
			{
				bms_dance_pts += 2;
			}
			else
			{
				bms_combo = -1;
			}

			bms_combo = std::min(10LL, bms_combo + 1);
			bms_combo_pts += bms_combo;

			bms_score =
				50000 * bms_combo_pts / bms_max_combo_pts
				+ 10000 * bms_dance_pts / max_notes;
		}

		void ScoreKeeper::update_lr2(ScoreKeeperJudgment judgment)
		{
			if ((!use_w0_for_ex2 && judgment <= SKJ_W1) || (use_w0_for_ex2 && judgment == SKJ_W0))
			{
				lr2_dance_pts += 10;
			}
			else if ((!use_w0_for_ex2 && judgment == SKJ_W2) || (use_w0_for_ex2 && judgment == SKJ_W1))
			{
				lr2_dance_pts += 5;
			}
			else if ((!use_w0_for_ex2 && judgment == SKJ_W3) || (use_w0_for_ex2 && judgment == SKJ_W2))
			{
				lr2_dance_pts += 1;
			}

			lr2_score = 20000 * lr2_dance_pts / max_notes;
		}

		int ScoreKeeper::getBMRank() const
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

		/*

		EXP^2 system

		The way this system works is similar to how combo scoring works in Beatmania.

		The max score for this system is 1,200,000 points, which is a normalized version of another score.

		The score that a note receives is based on your current "point combo".

		- W3 increases the point combo by  2.
		- W2 increases the point combo by  3.
		- W1 increases the point combo by  4.

		This point combo starts at the beginning of a song and has a maximum bound of 100.
		However, the point combo resets to a maximum of 96 after a note is judged, so a W3 cannot get more than 98 points, and a W2 cannot get more than 99.

		Also, the point combo resets to 0 if a note is missed.

		*/

		void ScoreKeeper::update_exp2(ScoreKeeperJudgment judgment)
		{
			if ((!use_w0_for_ex2 && judgment <= SKJ_W1) || (use_w0_for_ex2 && judgment == SKJ_W0))
			{
				exp_combo += 4;
				exp_hit_score += 2;
			}
			else if ((!use_w0_for_ex2 && judgment == SKJ_W2) || (use_w0_for_ex2 && judgment == SKJ_W1))
			{
				exp_combo += 3;
				exp_hit_score += 2;
			}
			else if ((!use_w0_for_ex2 && judgment == SKJ_W3) || (use_w0_for_ex2 && judgment == SKJ_W2))
			{
				exp_combo += 2;
				exp_hit_score += 1;
			}
			else if (use_w0_for_ex2 && judgment == SKJ_W3)
			{
				exp_combo = 1;
				exp_hit_score += 1;
			}
			else
			{
				exp_combo = 0;
			}

			exp_combo_pts += exp_combo;

			if (exp_combo > 96) exp_combo = 96;

			exp_score = 1.2e6 * exp_combo_pts / exp_max_combo_pts;
			exp3_score = 0.8e6 * exp_combo_pts / exp_max_combo_pts + 0.1e6 * exp_hit_score / max_notes;
		}

		void ScoreKeeper::update_osu(ScoreKeeperJudgment judgment)
		{
			int osu_bonus_multiplier = 0;

			switch (judgment)
			{
			case SKJ_W0:
				osu_points += 320;
				osu_accuracy += 300;
				osu_bonus_multiplier = 32;
				bonus_counter = std::min(100, bonus_counter + 2);
				break;
			case SKJ_W1:
				osu_points += 300;
				osu_accuracy += 300;
				osu_bonus_multiplier = 32;
				bonus_counter = std::min(100, bonus_counter + 1);
				break;
			case SKJ_W2:
				osu_points += 200;
				osu_accuracy += 200;
				osu_bonus_multiplier = 16;
				bonus_counter = std::max(0, bonus_counter - 8);
				break;
			case SKJ_W3:
				osu_points += 100;
				osu_accuracy += 100;
				osu_bonus_multiplier = 8;
				bonus_counter = std::max(0, bonus_counter - 24);
				break;
			case SKJ_W4:
				osu_points += 50;
				osu_accuracy += 50;
				osu_bonus_multiplier = 4;
				bonus_counter = std::max(0, bonus_counter - 44);
				break;
			case SKJ_MISS:
				osu_points += 0;
				bonus_counter = 0;
				break;
			default:
				break;
			}

			osu_bonus_points += osu_bonus_multiplier * sqrt(double(bonus_counter));

			osu_score = 500000 * ((osu_points + osu_bonus_points) / double(max_notes * 320));
		}

		void ScoreKeeper::update_ranks(ScoreKeeperJudgment judgment)
		{
			if (!use_w0_for_ex2 && judgment <= SKJ_W0) ++rank_w0_count;

			if ((!use_w0_for_ex2 && judgment <= SKJ_W1) || (use_w0_for_ex2 && judgment <= SKJ_W0)) ++rank_w1_count;
			if ((!use_w0_for_ex2 && judgment <= SKJ_W2) || (use_w0_for_ex2 && judgment <= SKJ_W1)) ++rank_w2_count;
			if (judgment <= SKJ_W3) ++rank_w3_count;

			long long rank_w0_pts = std::max(rank_w0_count * 2 - total_notes, 0LL);
			long long rank_w1_pts = std::max(rank_w1_count * 2 - total_notes, 0LL);
			long long rank_w2_pts = std::max(rank_w2_count * 2 - total_notes, 0LL);
			long long rank_w3_pts = std::max(rank_w3_count * 2 - total_notes, 0LL);

			rank_pts = rank_w0_pts + rank_w1_pts + rank_w2_pts + rank_w3_pts;
		}

		int ScoreKeeper::getRank() const
		{
			// find some way to streamline this.

			if (rank_pts == total_notes * 400 / 100) return 15;
			if (rank_pts >= total_notes * 380 / 100) return 14;
			if (rank_pts >= total_notes * 360 / 100) return 13;
			if (rank_pts >= total_notes * 340 / 100) return 12;
			if (rank_pts >= total_notes * 320 / 100) return 11;

			if (rank_pts >= total_notes * 300 / 100) return 10;
			if (rank_pts >= total_notes * 280 / 100) return 9;
			if (rank_pts >= total_notes * 260 / 100) return 8;
			if (rank_pts >= total_notes * 240 / 100) return 7;
			if (rank_pts >= total_notes * 220 / 100) return 6;
			if (rank_pts >= total_notes * 200 / 100) return 5;
			if (rank_pts >= total_notes * 180 / 100) return 4;
			if (rank_pts >= total_notes * 160 / 100) return 3;
			if (rank_pts >= total_notes * 140 / 100) return 2;
			if (rank_pts >= total_notes * 120 / 100) return 1;
			if (rank_pts >= total_notes * 100 / 100) return 0;
			if (rank_pts >= total_notes * 90 / 100) return -1;
			if (rank_pts >= total_notes * 80 / 100) return -2;
			if (rank_pts >= total_notes * 70 / 100) return -3;
			if (rank_pts >= total_notes * 60 / 100) return -4;
			if (rank_pts >= total_notes * 50 / 100) return -5;
			if (rank_pts >= total_notes * 40 / 100) return -6;
			if (rank_pts >= total_notes * 20 / 100) return -7;
			if (rank_pts > 0) return -8;
			return -9;
		}
	}
}