#include "Global.h"
#include "GraphObject2D.h"
#include "TrackNote.h"

using namespace VSRG;

TrackNote::TrackNote()
{
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
	Data.FractionKind = GetFractionKindBeat(frac);
}

Mat4 TrackNote::GetHoldPositionMatrix(const float &trackPosition) const
{
	float VerticalHoldBodyPos = b_pos + (b_pos_holdend - b_pos) / 2;
	return glm::translate(Mat4(), glm::vec3(trackPosition, VerticalHoldBodyPos, 14));
}

Mat4 TrackNote::GetHoldBodyMatrix(const float &noteWidth, const float &speedMultiplier) const
{
	float VertHBS = abs((b_pos_holdend - b_pos));
	return glm::scale(Mat4(), glm::vec3(noteWidth, VertHBS * speedMultiplier, 1)) * glm::translate(Mat4(), glm::vec3());
}

void TrackNote::AssignPosition(float Position, float endPosition)
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
	return glm::translate(Mat4(), glm::vec3(0, b_pos_holdend, 0));
}

Mat4 TrackNote::GetMatrix() const
{
	return glm::translate(Mat4(), glm::vec3(0, b_pos, 0));
}

float TrackNote::GetVertical() const
{
	return b_pos;
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
	return Data.EnabledHitFlags & EnabledFlag;
}

bool TrackNote::IsHeadEnabled() const
{
	return (Data.EnabledHitFlags & HeadEnabledFlag) != 0;
}

void TrackNote::Disable()
{
	Data.EnabledHitFlags &= ~EnabledFlag;
	DisableHead();
}

void TrackNote::DisableHead()
{
	Data.EnabledHitFlags &= ~HeadEnabledFlag;
}

void TrackNote::Hit()
{
	Data.EnabledHitFlags |= WasHitFlag;
}

bool TrackNote::WasNoteHit() const
{
	return (Data.EnabledHitFlags & WasHitFlag) != 0;
}

float TrackNote::GetVerticalHold() const
{
	return b_pos_holdend;
}

int TrackNote::GetSound() const
{
	return Data.Sound;
}

int TrackNote::GetFracKind() const
{
	return Data.FractionKind;
}

NoteData &TrackNote::GetNotedata()
{
	return Data;
}