#pragma once

/* Matrix size constants */

// 4:3
const uint16_t ScreenWidthDefault = 1024;
const uint16_t ScreenHeightDefault = 768;

enum KeyType
{
	// General stuff
	KT_Unknown,
	KT_Escape,
	KT_Select,
	KT_Enter,
	KT_BSPC,
	KT_SelectRight,
	KT_Up,
	KT_Down,
	KT_Left,
	KT_Right,
	KT_ReloadScreenScripts,
	KT_Debug,
	KT_ReloadCFG,

	// 7K specific
	KT_P1_SCRATCH_UP,
	KT_P1_SCRATCH_DOWN,
	KT_P2_SCRATCH_UP,
	KT_P2_SCRATCH_DOWN,
	KT_Key1,
	KT_Key2,
	KT_Key3,
	KT_Key4,
	KT_Key5,
	KT_Key6,
	KT_Key7,
	KT_Key8,
	KT_Key9,
	KT_Key10,
	KT_Key11,
	KT_Key12,
	KT_Key13,
	KT_Key14,
	KT_Key15,
	KT_Key16
};

extern const char* KeytypeNames[];




namespace Game {
	namespace VSRG
	{
		const auto DEFAULT_WAIT_TIME = 1.5;

		const int SCRATCH_1P_CHANNEL = 0;
		const int SCRATCH_2P_CHANNEL = 8;

		inline bool IsScratchLane(uint32_t lane) {
			return lane == SCRATCH_1P_CHANNEL || lane == SCRATCH_2P_CHANNEL;
		}

		// note type: 3 bits
		enum ENoteKind
		{
			NK_NORMAL,
			NK_FAKE,
			NK_MINE,
			NK_LIFT,
			NK_ROLL, // subtype of hold
			NK_INVISIBLE,
			NK_TOTAL
		};

		enum ChartType
		{
			// Autodecide - Not a value for anything other than PlayscreenParameters!
			TI_NONE = 0,
			TI_BMS = 1,
			TI_OSUMANIA = 2,
			TI_O2JAM = 3,
			TI_STEPMANIA = 4,
			TI_RAINDROP = 5,
			TI_RDAC = 6
		};


		// The values here must be consistent with the shaders!
		enum EHiddenMode
		{
			HM_NONE,
			HM_SUDDEN,
			HM_HIDDEN,
			HM_FLASHLIGHT,
		};

		const uint8_t MAX_CHANNELS = 16;

		enum ScoreKeeperJudgment
		{
			SKJ_NONE = -1, // no judgment.

			SKJ_W0 = 0, // Ridiculous / rainbow 300, for those programs that use it.

			SKJ_W1 = 1, // J_PERFECT / flashing J_GREAT
			SKJ_W2 = 2, // J_GREAT
			SKJ_W3 = 3, // Good
			SKJ_W4 = 4, // Bad
			SKJ_W5 = 5, // W5 is unused in beatmania.

			SKJ_MISS = 6, // Miss / Poor

			SKJ_HOLD_OK = 10, // OK, only used with DDR-style holds
			SKJ_HOLD_NG = 11, // NG
		};

		enum ScoreType
		{
			ST_RANK = 1, // raindrop rank scoring

			ST_EX = 2, // EX score
			ST_DP = 3, // DP score

			ST_IIDX = 10, // IIDX score
			ST_LR2 = 11, // LR2 score

			ST_SCORE = 20, // score out of 100m
			ST_OSUMANIA = 21, // osu!mania scoring
			ST_JB2 = 22, // jubeat^2 scoring

			ST_EXP = 30, // experimental scoring.
			ST_EXP3 = 31, // experimental scoring.

			ST_O2JAM = 40, // O2jam scoring.

			ST_COMBO = 100, // current combo
			ST_MAX_COMBO = 101, // max combo
			ST_NOTES_HIT = 102, // total notes hit
		};

		enum PercentScoreType
		{
			PST_RANK = 1, // raindrop rank score

			PST_EX = 2, // EX score
			PST_NH = 3, // % notes hit
			PST_ACC = 4, // Accuracy

			PST_OSU = 5, // osu!mania
		};

		const double SCORE_MAX = 100000000;

		enum LifeType {

			LT_AUTO = 0, // Only for PlayscreenParameters
			// actual groove type should be set by playing field from chart

			LT_GROOVE = 1, // Beatmania default lifebar
			LT_SURVIVAL = 2, // Beatmania hard mode
			LT_EXHARD = 3, // Beatmania EX hard mode
			LT_DEATH = 4, // Sudden death mode

			LT_EASY = 5, // Beatmania easy mode

			LT_STEPMANIA = 6, // Stepmania/DDR/osu!mania scoring mode.
			LT_NORECOV = 7, // DDR no recov. mode
			LT_O2JAM = 8,

			LT_BATTERY = 11, // DDR battery mode.
		};

		// Pacemakers.
		enum PacemakerType
		{
			PT_NONE = 0,

			PMT_A = 1,
			PMT_AA = 2,
			PMT_AAA = 3,

			PMT_50EX = 4,
			PMT_75EX = 5,
			PMT_85EX = 6,
			PMT_90EX = 7,

			PMT_RANK_ZERO = 20,
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

		enum TimingType
		{
			TT_TIME,
			TT_BEATS,
			TT_PIXELS
		};

		struct Difficulty;
		struct PlayerChartState;
		class ChartInfo;
		class Mechanics;
		class ScoreKeeper;

		struct PlayscreenParameters {
		private:
			struct SHiddenData {
				EHiddenMode		 Mode; // effective mode after upscroll adjustment
				float            Center; // in NDC
				float			 TransitionSize; // in NDC
				float			 CenterSize; // in NDC
			};
			
			void UpdateHidden(double JudgeY);
			SHiddenData Hidden;

			TimingType SetupGameSystem(
				std::shared_ptr<ChartInfo> TimingInfo, 
				ScoreKeeper* PlayerScoreKeeper);

			void SetupGauge(std::shared_ptr<ChartInfo> TimingInfo, ScoreKeeper* PlayerScoreKeeper);

			int Seed;
			bool IsSeedSet;

		public:

			// == Non Player Options ==
			// If true, assume difficulty is already loaded and is not just metadata
			int Preloaded;

			// Auto mode enabled if true.
			int Auto;

			// Selected starting measure (Preivew mode only)
			int32_t StartMeasure;

			// == Player options == 
			// If true, use upscroll (VSRG only)
			int Upscroll;

			// Fail disabled if true.
			int NoFail;

			// Selected hidden mode (VSRG only)
			int HiddenMode;

			// Music speed
			float Rate;

			// Scroll speed
			double UserSpeedMultiplier;

			// Randomizing mode -> 0 = Disabled, 1 = Per-Lane, 2 = Panic (unimplemented)
			int Random;

			// Gauge type (VSRG only)
			int32_t GaugeType;

			// Game System Type (VSRG only)
			int32_t SystemType;

			// Whether to interpret desired speed
			// as green number
			bool GreenNumber;

			// Whether to enable the use of strictest timing
			bool UseW0;

			PlayerChartState* Setup(
				double DesiredDefaultSpeed, 
				int SpeedType, 
				double Drift, 
				std::shared_ptr<VSRG::Difficulty> CurrentDiff);

			std::unique_ptr<Game::VSRG::Mechanics> PrepareMechanicsSet(
				std::shared_ptr<VSRG::Difficulty> CurrentDiff, 
				std::shared_ptr<Game::VSRG::ScoreKeeper> PlayerScorekeeper,
				double JudgeY);

			ScoreType GetScoringType() const;

			int GetHiddenMode();
			float GetHiddenCenter();
			float GetHiddenTransitionSize();
			float GetHiddenCenterSize();

			// Last used, or set, seed.
			int GetSeed();

			// Use this seed to shuffle.
			void SetSeed(int seed);

			// Unset the seed. Generate a new one.
			void ResetSeed();

			void deserialize(Json json);
			Json serialize() const;

			PlayscreenParameters() {
				Upscroll = false;
				Preloaded = false;
				Auto = false;
				NoFail = false;
				GreenNumber = false;
				UseW0 = false;
				HiddenMode = HM_NONE;
				StartMeasure = -1;
				Random = 0;
				Rate = 1;
				GaugeType = LT_AUTO;
				SystemType = VSRG::TI_NONE;
				UserSpeedMultiplier = 4;
				IsSeedSet = false;
			}
		};

		/* Vertical Space for a Measure.
		 A single 4/4 measure takes all of the playing field.
		 Increasing this will decrease multiplier resolution. */
		extern SkinMetric PLAYFIELD_SIZE;
		extern SkinMetric UNITS_PER_MEASURE;

		/* vsrg constants */

		enum ESpeedType
		{
			SPEEDTYPE_DEFAULT = -1,
			SPEEDTYPE_FIRST, // first, after SV
			SPEEDTYPE_MMOD, // maximum mod
			SPEEDTYPE_CMOD, // constant
			SPEEDTYPE_FIRSTBPM, // first, ignoring sv
			SPEEDTYPE_MODE, // most common
			SPEEDTYPE_MULTIPLIER // use target speed as multiplier directly
		};
	}
}

/* Program itself consts */
#define RAINDROP_WINDOWTITLE "raindrop ver: "
#define RAINDROP_VERSION "0.600"
#ifdef NDEBUG
#define RAINDROP_BUILDTYPE " "
#else
#define RAINDROP_BUILDTYPE " (debug) "
#endif

#define RAINDROP_VERSIONTEXT RAINDROP_VERSION RAINDROP_BUILDTYPE __DATE__

#include "BindingsManager.h"

