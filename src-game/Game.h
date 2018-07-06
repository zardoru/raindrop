

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

// The values here must be consistent with the shaders!
		enum EHiddenMode
		{
			HM_NONE,
			HM_SUDDEN,
			HM_HIDDEN,
			HM_FLASHLIGHT,
		};
        
extern const char* KeytypeNames[];


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