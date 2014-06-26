#include "Global.h"
#include "GraphObject2D.h"
#include "TrackNote.h"

using namespace VSRG;

TrackNote::TrackNote()
{
	Enabled = true;
	WasHit = false;
}

TrackNote::~TrackNote()
{
}

void TrackNote::AssignNotedata(const VSRG::NoteData &Notedata)
{
	Data = Notedata;
}

int GetFractionKindBeat(double frac);

/* calculate the beat snap for this fraction */
void TrackNote::AssignFraction(double frac)
{
	FractionKind = GetFractionKindBeat(frac);
}

void TrackNote::RecalculateBody(float trackPosition, float noteWidth, float noteSize, float speedMultiplier)
{
	hold_body_size = glm::translate(Mat4(), glm::vec3(trackPosition, 0, 14)) * glm::scale(Mat4(), glm::vec3(noteWidth, VerticalHoldBodySize * speedMultiplier * 2, 0));
}

void TrackNote::AssignPosition(Vec2 Position, Vec2 endPosition)
{
	b_pos = Position;
	final = glm::translate(Mat4(), glm::vec3(b_pos.x, b_pos.y, 0));

	if (Data.EndTime)
	{
		b_pos_holdend = endPosition;
		
		float VerticalHoldBodyPos = b_pos.y + (b_pos_holdend.y - b_pos.y) / 2;
		VerticalHoldBodySize = abs((b_pos_holdend.y - b_pos.y) / 2);

		hold_body = glm::translate(Mat4(), glm::vec3(b_pos.x, VerticalHoldBodyPos, 0));
		hold_final = glm::translate(Mat4(), glm::vec3(b_pos_holdend.x, b_pos_holdend.y, 0));
	}
}

bool TrackNote::IsHold() const
{
	return Data.EndTime != 0;
}

Mat4 TrackNote::GetHoldBodyMatrix() const
{
	return hold_body;
}

Mat4 TrackNote::GetHoldEndMatrix() const
{
	return hold_final;
}

Mat4 TrackNote::GetMatrix() const
{
	return final;
}

Mat4 TrackNote::GetHoldBodySizeMatrix() const
{
	return hold_body_size;
}

float TrackNote::GetVertical() const
{
	return b_pos.y;
}

void TrackNote::AddTime(double Time)
{
	Data.StartTime += Time;

	if (IsHold())
		Data.EndTime += Time;
}

double TrackNote::GetTimeFinal() const
{
	return max(Data.StartTime, Data.EndTime);
}

double TrackNote::GetStartTime() const
{
	return Data.StartTime;
}

bool TrackNote::IsEnabled() const
{
	return Enabled;
}

void TrackNote::Disable()
{
	Enabled = false;
}

void TrackNote::Hit()
{
	WasHit = true;
}

bool TrackNote::WasNoteHit() const
{
	return WasHit;
}

float TrackNote::GetVerticalHold() const
{
	return b_pos_holdend.y;
}

int TrackNote::GetSound() const
{
	return Data.Sound;
}

int TrackNote::GetFracKind() const
{
	return FractionKind;
}