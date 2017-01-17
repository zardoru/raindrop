#pragma once

/* Matrix size constants */

// 4:3
const uint16_t ScreenWidthDefault = 1024;
const uint16_t ScreenHeightDefault = 768;

// 16:9
const uint16_t ScreenWidthWidescreen = 1360;
const uint16_t ScreenHeightWidescreen = 768;

/* raindrop  .cur mode Consts */
namespace Game {
	namespace dotcur {
		const uint16_t PlayfieldWidth = 800;
		const uint16_t PlayfieldHeight = 600;
		const int16_t  ScreenOffset = 80;
		const float    CircleSize = 80.0f;

		const float	 LeniencyHitTime = 0.135f;
		const float	 HoldLeniencyHitTime = 0.1f;

		const float  LE_EXCELLENT = 0.03f;
		const float  LE_PERFECT = 0.05f;
		const float  LE_GREAT = 0.1f;

		enum Judgment
		{
			None,
			NG,
			Miss,
			OK,
			Bad,
			J_GREAT,
			J_PERFECT,
			J_EXCELLENT
		};

		struct EvaluationData
		{
			uint32_t MaxCombo;
			uint32_t NumNG;
			uint32_t NumOK;
			uint32_t NumMisses;
			uint32_t NumBads;
			uint32_t NumGreats;
			uint32_t NumPerfects;
			uint32_t NumExcellents;

			// Scoring
			uint32_t totalNotes;
			double dpScore;
			double dpScoreSquare;
		};
	}
}

float _ScreenDifference();

#define ScreenDifference _ScreenDifference()





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

	// raindrop specific
	KT_GameplayClick,

	// Editor specific
	KT_FractionDec,
	KT_FractionInc,
	KT_ChangeMode,
	KT_GridDec,
	KT_GridInc,
	KT_SwitchOffsetPrompt,
	KT_SwitchBPMPrompt,

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

extern char* KeytypeNames[];




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
			// Autodecide - Not a value for anything other than Parameters!
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
			ST_SCORE = 1, // raindrop's 7K scoring type.

			ST_EX = 2, // EX score
			ST_DP = 3, // DP score

			ST_IIDX = 10, // IIDX score
			ST_LR2 = 11, // LR2 score

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

			LT_AUTO = 0, // Only for Parameters
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

		struct Parameters {
			// If true, use upscroll (VSRG only)
			int Upscroll;

			// If true, enable Wave (VSRG only)
			int Wave;

			// If true, assume difficulty is already loaded and is not just metadata
			int Preloaded;

			// Fail disabled if true.
			int NoFail;

			// Auto mode enabled if true.
			int Auto;

			// Selected hidden mode (VSRG only)
			int HiddenMode;

			// Music speed
			float Rate;

			// Scroll speed
			double SpeedMultiplier;

			// Randomizing mode -> 0 = Disabled, 1 = Per-Lane, 2 = Panic (unimplemented)
			int Random;

			// Selected starting measure (Preivew mode only)
			int32_t StartMeasure;

			// Gauge type (VSRG only)
			int32_t GaugeType;

			// Game System Type (VSRG only)
			int32_t SystemType;

			Parameters() {
				Upscroll = false;
				Wave = false;
				Preloaded = false;
				Auto = false;
				NoFail = false;
				HiddenMode = HM_NONE;
				StartMeasure = -1;
				Random = 0;
				Rate = 1;
				GaugeType = LT_AUTO;
				SystemType = VSRG::TI_NONE;
				SpeedMultiplier = 4;
			}
		};

		/* Vertical Space for a Measure.
		 A single 4/4 measure takes all of the playing field.
		 Increasing this will decrease multiplier resolution. */
		const float MeasureBaseSpacing = 0.8f * ScreenHeightDefault;

		/* vsrg constants */

		enum ESpeedType
		{
			SPEEDTYPE_DEFAULT = -1,
			SPEEDTYPE_FIRST,
			SPEEDTYPE_MMOD,
			SPEEDTYPE_CMOD,
			SPEEDTYPE_FIRSTBPM,
			SPEEDTYPE_MODE
		};
	}
}

/* Program itself consts */
#define RAINDROP_WINDOWTITLE "raindrop ver: "
#define RAINDROP_VERSION "0.400"
#ifdef NDEBUG
#define RAINDROP_BUILDTYPE " "
#else
#define RAINDROP_BUILDTYPE " (debug) "
#endif

#define RAINDROP_VERSIONTEXT RAINDROP_VERSION RAINDROP_BUILDTYPE __DATE__

#include "BindingsManager.h"
#include "Configuration.h"
