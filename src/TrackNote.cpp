#include "Global.h"
#include "GraphObject2D.h"
#include "TrackNote.h"

TrackNote::TrackNote()
{
	Track = StartTime = EndTime = Measure = Fraction = 0;
}

TrackNote::~TrackNote()
{
}

void TrackNote::AssignTrack(int T)
{
	Track = T;
}

void TrackNote::AssignTime(float Start, float End)
{
	StartTime = Start;
	EndTime = End;
}

void TrackNote::AssignSongPosition(int _Measure, int _Fraction)
{
	Measure = _Measure;
	Fraction = _Fraction;
}

int TrackNote::GetMeasure()
{
	return Measure;
}

int TrackNote::GetFraction()
{
	return Fraction;
}

void TrackNote::AssignPosition(glm::vec2 Position)
{
	b_pos = Position;
	final = glm::translate(glm::mat4(), glm::vec3(b_pos.x, b_pos.y, 0));
}

void TrackNote::AssignSpeedMultiplier(float Mult)
{
	b_pos.y = b_pos.y * Mult;
	
}

glm::mat4& TrackNote::GetMatrix()
{
	return final;
}

float TrackNote::GetVertical() const
{
	return b_pos.y;
}

float TrackNote::GetTimeFinal()
{
	return std::max(StartTime, EndTime);
}

float TrackNote::GetStartTime()
{
	return StartTime;
}