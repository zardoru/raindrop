
#ifndef GAMECONSTS_H_
#define GAMECONSTS_H_

const uint16 ScreenWidth = 1024;
const uint16 ScreenHeight = 768;

/* dotcur Consts */
const uint16 PlayfieldWidth          = 800;
const uint16 PlayfieldHeight         = 600;
const int16  ScreenOffset            = 80;
const float  CircleSize              = 80.0f;

const float	 LeniencyHitTime         = 0.135f;
const float	 HoldLeniencyHitTime     = 0.1f;

const float  DotcurExcellentLeniency = 0.03f;
const float  DotcurPerfectLeniency   = 0.05f;
const float  DotcurGreatLeniency	 = 0.1f;

/* 7k Consts */
// Ratio of how much of the screen is for the Gear, assuming 4:3 ratio
const float GearHeight  = 0.2f * ScreenHeight;
const float GearWidth   = 0.35f * ScreenWidth;
const float GearStartX  = (ScreenWidth - GearWidth) / 2;

// Vertical Space for a Measure.
const float MeasureBaseSpacing = 0.4f * ScreenHeight;

const float LeniencyHitTime7K = 0.050f; // 50 ms
const float GreatLeniency7K = 0.040f;
const float PerfectLeniency7K = 0.025f;
const float ExcellentLeniency7K = 0.016f;

enum Judgement
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

struct AccuracyData7K
{
	double Accuracy;
	unsigned int TotalNotes;
};

enum KeyEventType
{
	KE_None,
	KE_Press,
	KE_Release
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
	KT_GoToEditMode,
	KT_Up,
	KT_Down,
	KT_Left,
	KT_Right,

	// dotcur specific
	KT_Hit,
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
	KT_Key14
};

enum ModeType
	{
		MODE_DOTCUR,
		MODE_7K
	};

#define DOTCUR_WINDOWTITLE "dC ver: "
#define DOTCUR_VERSION "0.4.1"
#ifdef NDEBUG
#define DOTCUR_BUILDTYPE " "
#else
#define DOTCUR_BUILDTYPE " (debug) "
#endif


#define DOTCUR_VERSIONTEXT DOTCUR_VERSION DOTCUR_BUILDTYPE __DATE__

#include "BindingsManager.h"

float _ScreenDifference();

#define ScreenDifference _ScreenDifference()

#endif