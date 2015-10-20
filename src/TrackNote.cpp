#include "GameGlobal.h"
#include "TrackNote.h"
#include <glm/gtc/matrix_transform.inl>

using namespace VSRG;

TrackNote::TrackNote()
{
	EnabledHitFlags = EnabledFlag | HeadEnabledFlag;
}

TrackNote::~TrackNote()
{
}

float TrackNote::GetHoldSize() const
{
	return abs((b_pos_holdend - b_pos));
}

float TrackNote::GetHoldEndVertical()
{
	return b_pos_holdend;
}

void TrackNote::FailHit()
{
	EnabledHitFlags |= FailedHitFlag;
}


void TrackNote::MakeInvisible()
{
	EnabledHitFlags |= InvisibleFlag;
}

bool TrackNote::FailedHit() const
{
	return (EnabledHitFlags & FailedHitFlag) != 0;
}

void TrackNote::AssignNotedata(const VSRG::NoteData &Notedata)
{
	Time = Notedata.StartTime;
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

void TrackNote::AssignPosition(float Position, float endPosition)
{
	b_pos = Position;
	b_pos_holdend = endPosition;
}

bool TrackNote::IsHold() const
{
	return EndTime != 0;
}

float TrackNote::GetVertical() const
{
	return b_pos;
}

void TrackNote::AddTime(double Time)
{
	this->Time += Time;

	if (IsHold())
		EndTime += Time;
}

double TrackNote::GetTimeFinal() const
{
	return max(Time, EndTime);
}

double TrackNote::GetStartTime() const
{
	return Time;
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
	return b_pos + GetHoldSize() / 2;
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
	return Time;
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
	return NoteKind != NK_INVISIBLE && !(EnabledHitFlags & InvisibleFlag);
}