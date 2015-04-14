#include "GameGlobal.h"
#include "Sprite.h"
#include "TrackNote.h"

using namespace VSRG;

TrackNote::TrackNote()
{
	EnabledHitFlags = EnabledFlag | HeadEnabledFlag;
}

TrackNote::~TrackNote()
{
}

void TrackNote::AssignNotedata(const VSRG::NoteData &Notedata)
{
	StartTime = Notedata.StartTime;
	EndTime = Notedata.EndTime;
	Sound = Notedata.Sound;
	NoteKind = Notedata.NoteKind;
}

int GetFractionKindBeat(double frac);

/* calculate the beat snap for this fraction */
void TrackNote::AssignFraction(double frac)
{
	FractionKind = GetFractionKindBeat(frac);
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
	return EndTime != 0;
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
	StartTime += Time;

	if (IsHold())
		EndTime += Time;
}

double TrackNote::GetTimeFinal() const
{
	return max(StartTime, EndTime);
}

double TrackNote::GetStartTime() const
{
	return StartTime;
}

bool TrackNote::IsEnabled() const
{
	return EnabledHitFlags & EnabledFlag;
}

bool TrackNote::IsHeadEnabled() const
{
	return (EnabledHitFlags & HeadEnabledFlag) != 0;
}

void TrackNote::Disable()
{
	EnabledHitFlags &= ~EnabledFlag;
	DisableHead();
}

void TrackNote::DisableHead()
{
	EnabledHitFlags &= ~HeadEnabledFlag;
}

void TrackNote::Hit()
{
	EnabledHitFlags |= WasHitFlag;
}

bool TrackNote::WasNoteHit() const
{
	return (EnabledHitFlags & WasHitFlag) != 0;
}

float TrackNote::GetVerticalHold() const
{
	return b_pos_holdend;
}

int TrackNote::GetSound() const
{
	return Sound;
}

int TrackNote::GetFracKind() const
{
	return FractionKind;
}

double &TrackNote::GetDataStartTime()
{
	return StartTime;
}

double &TrackNote::GetDataEndTime()
{
	return EndTime;
}

uint16 &TrackNote::GetDataSound()
{
	return Sound;
}

uint8  TrackNote::GetDataNoteKind()
{
	return NoteKind;
}

uint8  TrackNote::GetDataFractionKind()
{
	return FractionKind;
}

bool TrackNote::IsJudgable() const
{
	return NoteKind != NK_INVISIBLE && NoteKind != NK_FAKE;
}

bool TrackNote::IsVisible() const
{
	return NoteKind != NK_INVISIBLE;
}