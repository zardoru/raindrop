#include "Global.h"
#include "Song.h"

Song7K::Song7K()
{
	MeasureLength = 4;
	LeadInTime = 1.5;
}

Song7K::~Song7K()
{
}

void Song7K::Process()
{
	/* 
		We'd like to build the notes' position from 0 to infinity, 
		however the real "zero" position would be the judgement line
		in other words since "up" is negative relative to 0
		and 0 is the judgement line
		position would actually be
		judgeline - positiveposition
		and positiveposition would just be
		measure * measuresize + fraction * fractionsize
	*/
}