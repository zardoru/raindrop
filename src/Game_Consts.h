
#ifndef GAMECONSTS_H_
#define GAMECONSTS_H_

const uint16 ScreenWidth = 1024;
const uint16 ScreenHeight = 768;
const uint16 PlayfieldWidth = 800;
const uint16 PlayfieldHeight = 600;
const int16 ScreenOffset = 80;
const float CircleSize = 80.0f;
const float		   LeniencyHitTime = 0.35f;
const float		   HoldLeniencyHitTime = 0.1f;

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
};

enum KeyEventType
{
	KE_None,
	KE_Press,
	KE_Release
};

enum KeyType
{
	KT_Unknown,
	KT_Escape,
	KT_Hit,
	KT_GameplayClick,
	KT_Select,
	KT_SelectRight,
	KT_GoToEditMode,
	KT_Up,
	KT_Down,
	KT_Left,
	KT_Right,

	// Editor specific
	KT_FractionDec,
	KT_FractionInc,
	KT_ChangeMode,

};

#define DOTCUR_WINDOWTITLE "dC ver: "
#define DOTCUR_VERSION "underground 1"
#ifdef NDEBUG
#define DOTCUR_BUILDTYPE ""
#else
#define DOTCUR_BUILDTYPE " (debug)"
#endif


#define DOTCUR_VERSIONTEXT DOTCUR_VERSION DOTCUR_BUILDTYPE

#include "BindingsManager.h"

float _ScreenDifference();

#define ScreenDifference _ScreenDifference()

#endif