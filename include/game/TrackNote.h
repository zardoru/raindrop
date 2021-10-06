#pragma once

#include "Timing.h"

namespace rd {
    // REMEMBER WE MUST BE 8-BYTE ALIGNED. It's either 16, 24 or 32...
    // note data: 28 bytes
    // Going to be 32 bytes on its own with alignment...
        struct NoteData
        {
            // start time and end time are 8 bytes each - 16 bytes.
            double StartTime, EndTime;
            uint32_t Sound; // Do we really need more than this?
            uint32_t TailSound;
            uint8_t NoteKind; // To be used with ENoteKind.
            // 7 bytes wasted

            NoteData()
            {
                StartTime = EndTime = 0;
                Sound = TailSound = 0;
                NoteKind = NK_NORMAL;
            }

            NoteData(double _StartTime, double _EndTime) : NoteData() {
                StartTime = _StartTime;
                EndTime = _EndTime;
            }
        };

		class TrackNote : public TimedEvent<TrackNote, double>
		{
		private:
			// 16 bytes (Implied 8 with inherited TimedEvent)
			double EndTime;

			// 16 bytes
			double b_pos;
			double b_pos_holdend;

			// 8 bytes
			uint32_t Sound;
			uint32_t TailSound;

			// 3 bytes
			uint8_t NoteKind; // To be used with ENoteKind.
			uint8_t FractionKind;

			// The only real state actually tracked by the note
			uint8_t EnabledHitFlags;

			// 48 bytes aligned
		public:
			TrackNote();
			explicit TrackNote(const NoteData &Data);

			// Build this tracknote from this NoteData.
			void AssignNotedata(const NoteData &Data);

			// az: These return by references are completely useless and kinda stupid.

			// Get the start time.
			double &GetDataStartTime();

			// Get the actual end time regardless of start time.
			double &GetDataEndTime();

			// Get what sound this note has. (Redundant?)
			uint32_t &GetDataSound();

			// Get the kind of head this note has.
			uint8_t  GetDataNoteKind() const;

			// Get what fraction of a beat this note belongs to.
			uint8_t  GetDataFractionKind();

			// Set this note's position on the vertical track.
			void AssignPosition(double Position, double endPosition = 0);

			// Assign a fraction of a beat to this note.
			void AssignFraction(double frc); // frc = fraction of a beat

			// Mark this note/hold head as hit.
			void Hit();

			// Add this much drift to the note. Doesn't reset.
			void AddTime(double Time);

			// Disable note for judgment completely. Takes it out of update/press/release/scratch events etc..
			void Disable();

			// Disable the head of this note. Leaves the option of hitting the tail if not disabled.
			// It's a mechanics flag - it still gets updated/pressed/released etc...
			void DisableHead();

			// Get the position on the track of the note/hold head.
			float GetVertical() const;

			// Get the position on the track of the hold's tail.
			float GetVerticalHold() const;

			// Get whether this note is a hold.
			bool IsHold() const;

			// Get whether this note can be judged.
			bool IsEnabled() const;

			// Get whether the head of this note (if a hold) is enabled.
			bool IsHeadEnabled() const;

			// Get whether this note was hit on the head.
			bool WasHit() const;

			// Get whether this note is a judgable kind of note. Doesn't depend on failure state.
			// It also doesn't apply mechanical rules (i.e. hit notes can't be judged twice)
			bool IsJudgable() const;

			// Get whether this note should be drawn - independant of whether it was hit.
			bool IsVisible() const;

			// Get the sound associated to this note.
			uint32_t GetSound() const;

			// Get the sound associated to this note's tail.
			uint32_t GetTailSound() const;

			// Get the maximum between the start and end times. If not a hold, Start = End.
			double GetEndTime() const;

			// Get the start time of this note. If not a hold, Start = End.
			double GetStartTime() const;

			// Get the beat fraction kind.
			int GetFracKind() const;

			~TrackNote();

			// Get the size of this hold on the unmodified track.
			float GetHoldSize() const;

			// Get the position of the tail of this hold on the unmodified track.
			float GetHoldEndVertical();

			// Informative flags. For use in mechanics.
			// Mark this object as failed. Used only to have the additional failed state.
			void FailHit();

			// Get whether this object was marked as failed.
			bool FailedHit() const;

			// Make this note invisible - That is to say, make no attempt at drawing it.
			void MakeInvisible();
			void RemoveSound();

			// Reset this note's state keeping timing/notetype information
			void Reset();
		};

		inline bool operator<(const TrackNote& tn, double time)
		{
			return tn.GetStartTime() < time;
		}
}