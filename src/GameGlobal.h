#include "Global.h"

#ifndef GAMEGLOBAL_H_
#define GAMEGLOBAL_H_

/* Matrix size constants */

// 4:3
const uint16 ScreenWidthDefault = 1024;
const uint16 ScreenHeightDefault = 768;

// 16:9
const uint16 ScreenWidthWidescreen = 1360;
const uint16 ScreenHeightWidescreen = 768;

/* raindrop  .cur mode Consts */
const uint16 PlayfieldWidth          = 800;
const uint16 PlayfieldHeight         = 600;
const int16  ScreenOffset            = 80;
const float  CircleSize              = 80.0f;

const float	 LeniencyHitTime         = 0.135f;
const float	 HoldLeniencyHitTime     = 0.1f;

const float  DotcurExcellentLeniency = 0.03f;
const float  DotcurPerfectLeniency   = 0.05f;
const float  DotcurGreatLeniency	 = 0.1f;

float _ScreenDifference();

#define ScreenDifference _ScreenDifference()

enum Judgment
{
	None,
	NG,
	Miss,
	OK,
	Bad,
	Great,
	Perfect,
	Excellent
};

struct EvaluationData
{
	uint32 MaxCombo;
	uint32 NumNG;
	uint32 NumOK;
	uint32 NumMisses;
	uint32 NumBads;
	uint32 NumGreats;
	uint32 NumPerfects;
	uint32 NumExcellents;

	// Scoring
	uint32 totalNotes;
	double dpScore;
	double dpScoreSquare;
};


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


/* vsrg constants */

enum ESpeedType {
	SPEEDTYPE_DEFAULT = -1,
	SPEEDTYPE_FIRST,
	SPEEDTYPE_MMOD,
	SPEEDTYPE_CMOD,
	SPEEDTYPE_FIRSTBPM,
} ;

// The values here must be consistent with the shaders!
enum EHiddenMode {
	HIDDENMODE_NONE = 0,
	HIDDENMODE_SUDDEN = 1,
	HIDDENMODE_HIDDEN = 2,
	HIDDENMODE_FLASHLIGHT = 3,
} ;

namespace VSRG {

	// note type: 3 bits
	enum ENoteKind {
		NK_NORMAL,
		NK_FAKE,
		NK_MINE,
		NK_LIFT,
		NK_ROLL, // subtype of hold
		NK_INVISIBLE,
		NK_TOTAL
	};

	const uint8 MAX_CHANNELS = 16;
}

struct GameParameters {
	// If true, use upscroll
	int Upscroll;

	// If true, enable Wave
	int Wave;

	// If true, assume difficulty is already loaded and is not just metadata
	int Preloaded;

	// Fail disabled if true.
	int NoFail;

	// Auto mode enabled if true.
	int Auto;

	// Selected hidden mode
	int HiddenMode;

	// Music speed
	float Rate;

	// Selected starting measure
	int32 StartMeasure;

	GameParameters() {
		Upscroll = false;
		Wave = false;
		Preloaded = false;
		Auto = false;
		NoFail = false;
		HiddenMode = HIDDENMODE_NONE;
		StartMeasure = -1;
		Rate = 1;
	}
};

// Vertical Space for a Measure. A single 4/4 measure takes all of the playing field. Increasing this will decrease multiplier resolution.
const float MeasureBaseSpacing = 0.8f * ScreenHeightDefault;

/* Program itself consts */
#define RAINDROP_WINDOWTITLE "raindrop ver: "
#define RAINDROP_VERSION "0.250"
#ifdef NDEBUG
#define RAINDROP_BUILDTYPE " "
#else
#define RAINDROP_BUILDTYPE " (debug) "
#endif


#define RAINDROP_VERSIONTEXT RAINDROP_VERSION RAINDROP_BUILDTYPE __DATE__

#include "BindingsManager.h"
#include "Configuration.h"

#endif
