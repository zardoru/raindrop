
#ifndef GAMECONSTS_H_
#define GAMECONSTS_H_

const unsigned int ScreenWidth = 1024;
const unsigned int ScreenHeight = 768;
const unsigned int PlayfieldWidth = 800;
const unsigned int PlayfieldHeight = 600;
const unsigned int CircleSize = 64;
const unsigned int ScreenOffset = 80;
const float		   LeniencyHitTime = 0.35;
const float		   HoldLeniencyHitTime = 0.1;

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
	unsigned int MaxCombo;
	unsigned int NumNG;
	unsigned int NumOK;
	unsigned int NumMisses;
	unsigned int NumBads;
	unsigned int NumGreats;
	unsigned int NumPerfects;
	unsigned int NumExcellents;
};

#endif