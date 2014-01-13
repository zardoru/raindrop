#include "Global.h"
#include "GraphObject2D.h"
#include "TrackNote.h"

TrackNote::TrackNote()
{
	Track = StartTime = EndTime = 0;
	Enabled = true;
	WasHit = false;
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

void TrackNote::RecalculateBody(float noteWidth, float noteSize, float speedMultiplier)
{
	hold_body_size = glm::scale(glm::mat4(), glm::vec3(noteWidth, VerticalHoldBodySize * speedMultiplier * 2, 0));
	//hold_body = glm::translate(glm::mat4(), glm::vec3(b_pos.x, VerticalHoldBodyPos, 0));
}

glm::mat4& TrackNote::GetHoldBodySizeMatrix()
{
	return hold_body_size;
}

void TrackNote::AssignPosition(glm::vec2 Position, glm::vec2 endPosition)
{
	b_pos = Position;
	final = glm::translate(glm::mat4(), glm::vec3(b_pos.x, b_pos.y, 0));

	if (EndTime)
	{
		b_pos_holdend = endPosition;
		
		VerticalHoldBodyPos = b_pos.y + (b_pos_holdend.y - b_pos.y) / 2;
		VerticalHoldBodySize = abs((b_pos_holdend.y - b_pos.y) / 2);

		hold_body = glm::translate(glm::mat4(), glm::vec3(b_pos.x, VerticalHoldBodyPos, 0));
		hold_final = glm::translate(glm::mat4(), glm::vec3(b_pos_holdend.x, b_pos_holdend.y, 0));
	}
}

bool TrackNote::IsHold() const
{
	return EndTime != 0;
}

glm::mat4& TrackNote::GetHoldBodyMatrix()
{
	return hold_body;
}

glm::mat4& TrackNote::GetHoldEndMatrix()
{
	return hold_final;
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

bool TrackNote::IsEnabled() const
{
	return Enabled;
}

void TrackNote::Disable()
{
	Enabled = false;
}

uint32 TrackNote::GetTrack() const
{
	return Track;
}

void TrackNote::Hit()
{
	WasHit = true;
}

bool TrackNote::WasNoteHit() const
{
	return WasHit;
}