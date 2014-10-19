#include "ScoreKeeper.h"

class ScoreKeeper7K {

	public:

		ScoreKeeper7K();
		ScoreKeeper7K(double judge_window_scale);

		~ScoreKeeper7K();

		void init();

		void setMaxNotes(int notes);
		void setLifeTotal(double total);
		void setJudgeRank(int rank);

		int getMaxNotes();
		int getTotalNotes();
		void setAccMin(int ms);
		void setAccMax(int ms);
		void setEX2(int ms);
		void setEX1(int ms);
		void setDP2(int ms);
		void setDP1(int ms);

		int getJudgmentCount(ScoreKeeperJudgment Judge);
		std::string getHistogram();

		ScoreKeeperJudgment hitNote(int ms);
		void missNote(bool auto_hold_miss, bool early_miss);

		double getAccMax();

		int getJudgmentWindow(ScoreKeeperJudgment judgment);
		double getMissCutoff();
		double getEarlyMissCutoff();

		int getScore(ScoreType score_type);
		float getPercentScore(PercentScoreType score_type);

		int getLifebarUnits(LifeType lifebar_unit_type);
		float getLifebarAmount(LifeType lifebar_amount_type);

		int getPacemakerDiff(PacemakerType pmt);

		int getRank(); // returns a number from -9 to 9

		void reset();

	private:

		double score; // standard score.
		double sc_score;
		double sc_sc_score;

	/*
		Rank scoring
	*/

		long long rank_w1_count;
		long long rank_w2_count;
		long long rank_w3_count;

		long long rank_pts; // rank scoring

		void update_ranks(int ms);

		long long max_notes;

	/*
		BMS scoring
	*/

		long long ex_score;

		long long bms_combo;
		long long bms_combo_pts;
		long long bms_max_combo_pts;

		long long bms_dance_pts;
		long long bms_score;

		long long lr2_dance_pts;
		long long lr2_score;

		void update_bms(int ms, bool hit);
		void update_lr2(int ms, bool hit);
		
	/*
		osu!
	*/

		void set_osu_judgment(int ms, ScoreKeeperJudgment judgment);


	/*
		experimental
	*/

		long long exp_combo;
		long long exp_combo_pts;
		long long exp_max_combo_pts;

		double exp_score;

		void update_exp2(int ms);

	/*
		misc.
	*/

		long long notes_hit; // notes hit %.
		long long total_notes;


		long long dp_score; // DDR dance-point scoring
		long long dp_dp_score;


		long long combo;
		long long max_combo;

		double total_sqdev; // accuracy scoring
		double accuracy;

		double accuracy_percent(float ms);



		// lifebar data.

		double lifebar_total;
		

		double lifebar_groove;
		double lifebar_groove_increment;
		double lifebar_groove_decrement;

		double lifebar_survival;
		double lifebar_survival_increment;
		double lifebar_survival_decrement;

		double lifebar_exhard;
		double lifebar_exhard_increment;
		double lifebar_exhard_decrement;

		double lifebar_death;

		double lifebar_easy;
		double lifebar_easy_increment;
		double lifebar_easy_decrement;

		// judgment information
		double judgment_time[9];
		double judgment_amt[9];
		double judge_window_scale;
		
		void set_timing_windows();
		
		// miss thresholds; notes hit outside here count as misses.
		double miss_threshold;
		double earlymiss_threshold;

		double histogram[255]; // records from -127 to +127 ms.

		// no-recovery modes.
		double lifebar_battery;

		long long lifebar_battery_lives;

		// scoring parameters.
		long long ACC_MIN, ACC_MIN_SQ;
		long long ACC_MAX, ACC_MAX_SQ;

};
