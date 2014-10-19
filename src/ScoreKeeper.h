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

};

enum ScoreType{

	ST_SCORE = 1, // raindrop's 7K scoring type.

	ST_EX = 2, // EX score
	ST_DP = 3, // DP score

	ST_IIDX = 10, // IIDX score
	ST_LR2 = 11, // LR2 score

	ST_OSU = 21, // osu!mania scoring
	ST_JB2 = 22, // jubeat^2 scoring

	ST_EXP = 30, // experimental scoring.

	ST_COMBO = 100, // current combo
	ST_MAX_COMBO = 101, // max combo
	ST_NOTES_HIT = 102, // total notes hit

};

enum PercentScoreType{

	PST_RANK = 1, // raindrop rank score

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




// Pacemakers.
enum PacemakerType{

	PT_NONE = 0,
	
	PMT_A = 1,
	PMT_AA = 2,
	PMT_AAA = 3,

	PMT_50EX = 4,
	PMT_75EX = 5,
	PMT_85EX = 6,
	PMT_90EX = 7,

	PMT_RANK_P1 = 21,
	PMT_RANK_P2 = 22,
	PMT_RANK_P3 = 23,
	PMT_RANK_P4 = 24,
	PMT_RANK_P5 = 25,
	PMT_RANK_P6 = 26,
	PMT_RANK_P7 = 27,
	PMT_RANK_P8 = 28,
	PMT_RANK_P9 = 29,

	PMT_B = 31,
	PMT_C = 32,
	PMT_D = 33,
	PMT_E = 34,
	PMT_F = 35,

};
