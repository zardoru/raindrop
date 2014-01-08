#include "Global.h"
#include "GraphObject2D.h"
#include "TrackNote.h"

TrackNote::TrackNote()
{
	Track = StartTime = EndTime = 0;
}

TrackNote::~TrackNote()
{
}

void TrackNote::AssignTrack(int T)
{
	Track = T;
}

void TrackNote::AssignTime(double Start, double End)
{
	StartTime = Start;
	EndTime = End;
}

void TrackNote::AssignSongPosition(double _BeatStart, double _BeatEnd)
{
	BeatStart = _BeatStart;
	BeatEnd = _BeatEnd;
}

double TrackNote::GetBeatStart() const
{
	return BeatStart;
}

double TrackNote::GetBeatEnd() const
{
	return BeatEnd;
}

void TrackNote::AddTime(double Time)
{
	StartTime += Time;

	if (EndTime) // Actually using this?
		EndTime += Time;
}

void TrackNote::AssignPosition(glm::vec2 Position)
{
	b_pos = Position;
	final = glm::translate(glm::mat4(), glm::vec3(b_pos.x, b_pos.y, 0));
}

glm::mat4& TrackNote::GetMatrix()
{
	return final;
}

float TrackNote::GetVertical() const
{
	return b_pos.y;
}

double TrackNote::GetTimeFinal() const
{
	return std::max(StartTime, EndTime);
}

double TrackNote::GetStartTime() const
{
	return StartTime;
}