// A class dedicated to keeping score.

enum ScoreKeeperJudgment{
	
	SKJ_NONE = 0, // no judgment.
	
	SKJ_W1 = 1, // Perfect / flashing Great
	SKJ_W2 = 2, // Great
	SKJ_W3 = 3, // Good
	SKJ_W4 = 4, // Bad
	SKJ_W5 = 5, // W5 is unused in beatmania.
	SKJ_MISS = 6, // Miss / Poor
	SKJ_HOLD_OK = 10, // OK, only used with DDR-style holds
	SKJ_HOLD_NG = 11, // NG
	SKJ_AMOUNT
};

enum ScoreType{
	
	ST_SCORE = 1, // dotCur's 7K scoring type.
	ST_EX = 2, // EX score
	ST_DP = 3, // DP score

	ST_COMBO = 10, // current combo
	ST_MAX_COMBO = 11, // max combo
	ST_NOTES_HIT = 12, // total notes hit
	
	ST_OSU = 21, // osu!mania scoring
	ST_JB2 = 22, // jubeat^2 scoring
	
};

enum PercentScoreType{
	
	PST_EX = 2, // EX score
	PST_NH = 3, // % notes hit
	PST_ACC = 4 // Accuracy

};

const double SCORE_MAX = 100000000;

enum LifeType{

	LT_GROOVE = 1, // Beatmania default score
	LT_SURVIVAL = 2, // Beatmania hard mode
	LT_EXHARD = 3, // Beatmania EX hard mode
	LT_DEATH = 4, // Sudden death mode

	LT_EASY = 5, // Beatmania easy mode

	LT_NORECOV = 6, // DDR no recov. mode
	LT_BATTERY = 7, // DDR battery mode.

};

class ScoreKeeper7K {

	public:

		ScoreKeeper7K();
		~ScoreKeeper7K();
		
		void setMaxNotes(int notes);
		void setAccMin(int ms);
		void setAccMax(int ms);
		void setEX2(int ms);
		void setEX1(int ms);
		void setDP2(int ms);
		void setDP1(int ms);
		
		ScoreKeeperJudgment hitNote(int ms);
		void missNote(bool auto_hold_miss);

		double getAccCutoff();
		double getAccMax();

		int getScore(ScoreType score_type);
		float getPercentScore(PercentScoreType score_type);
		
		int getLifebarUnits(LifeType lifebar_unit_type);
		float getLifebarAmount(LifeType lifebar_amount_type);

		void reset();

	private:

		// scoring parameters.
		int ACC_MIN, ACC_MIN_SQ;
		int ACC_MAX, ACC_MAX_SQ;
		int ACC_CUTOFF;

		int EX2, EX1;
		int DP2, DP1;

		// scoring number data.

		int max_notes;

		double score; // standard score.
		double sc_score;
		double sc_sc_score;

		int notes_hit;
		int total_notes;

		int ex_score;
		
		int dp_score;
		int dp_dp_score;

		int combo;
		int max_combo;

		double total_sqdev;
		double accuracy;

		double accuracy_percent(float ms);

		// lifebar data.
		
		double lifebar_groove;
		double lifebar_groove_increment;

		double lifebar_survival;
		double lifebar_survival_increment;

		double lifebar_exhard;
		double lifebar_exhard_increment;

		double lifebar_death;

		double lifebar_easy;
		double lifebar_easy_increment;
		
		// no-recovery modes.
		
		double lifebar_battery;
		int lifebar_battery_lives;

		double judgement_time[SKJ_AMOUNT];
		double judgement_amt[SKJ_AMOUNT];
};
