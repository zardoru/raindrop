#include "pch.h"

#include "GameGlobal.h"
#include "TrackNote.h"
//#include <glm/gtc/matrix_transform.inl>

int GetFractionKindBeat(double frac);

namespace Game {
	namespace VSRG {

		TrackNote::TrackNote()
		{
			Reset();
		}

		TrackNote::~TrackNote()
		{
		}

		void TrackNote::Reset()
		{
			EnabledHitFlags = EnabledFlag | HeadEnabledFlag;
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

		void TrackNote::RemoveSound()
		{
			Sound = 0;
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


		/* calculate the beat snap for this fraction */
		void TrackNote::AssignFraction(double frac)
		{
			FractionKind = GetFractionKindBeat(frac);
		}

		void TrackNote::AssignPosition(double Position, double endPosition)
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

		double TrackNote::GetEndTime() const
		{
			return std::max(Time, EndTime);
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

		bool TrackNote::WasHit() const
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

		uint16_t &TrackNote::GetDataSound()
		{
			return Sound;
		}

		uint8_t  TrackNote::GetDataNoteKind()
		{
			return NoteKind;
		}

		uint8_t  TrackNote::GetDataFractionKind()
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
	}
}