
#ifndef GAMECONSTS_H_
#define GAMECONSTS_H_

const uint16 ScreenWidth = 1024;
const uint16 ScreenHeight = 768;
const uint16 PlayfieldWidth = 800;
const uint16 PlayfieldHeight = 600;
const uint16 CircleSize = 80;
const int16 ScreenOffset = 80;
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

float _ScreenDifference();

#define ScreenDifference _ScreenDifference()

#endif