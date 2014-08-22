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

Mat4 TrackNote::GetHoldPositionMatrix(const float &trackPosition) const
{
	float VerticalHoldBodyPos = b_pos.y + (b_pos_holdend.y - b_pos.y) / 2;
	return glm::translate(Mat4(), glm::vec3(trackPosition, VerticalHoldBodyPos, 14));
}

Mat4 TrackNote::GetHoldBodyMatrix(const float &noteWidth, const float &speedMultiplier) const
{
	float VertHBS = abs((b_pos_holdend.y - b_pos.y));
	return glm::scale(Mat4(), glm::vec3(noteWidth, VertHBS * speedMultiplier, 1)) * glm::translate(Mat4(), glm::vec3());
}

void TrackNote::AssignPosition(Vec2 Position, Vec2 endPosition)
{
	b_pos = Position;
	b_pos_holdend = endPosition;
}

bool TrackNote::IsHold() const
{
	return Data.EndTime != 0;
}

Mat4 TrackNote::GetHoldEndMatrix() const
{
	return glm::translate(Mat4(), glm::vec3(b_pos_holdend.x, b_pos_holdend.y, 0));
}

Mat4 TrackNote::GetMatrix() const
{
	return glm::translate(Mat4(), glm::vec3(b_pos.x, b_pos.y, 0));
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