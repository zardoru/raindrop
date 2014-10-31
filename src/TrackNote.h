

#ifndef TAPNOTE_H_
#define TAPNOTE_H_

namespace VSRG
{
	const unsigned char EnabledFlag = 1 << 0;
	const unsigned char WasHitFlag = 1 << 1;

	// note type: 3 bits
	enum ENoteKind {
		NK_NORMAL,
		NK_FAKE,
		NK_MINE,
		NK_LIFT,
		NK_ROLL // subtype of hold
	};

	// REMEMBER WE MUST BE 8-BYTE ALIGNED. It's either 16, 24 or 32...
	// note data: 20 bytes
	// Going to be 24 bytes on its own with alignment...
	struct NoteData
	{
		// start time and end time are 8 bytes each - 16 bytes.
		double StartTime, EndTime;
		uint16 Sound; // Do we really need more than 65536 sounds?
		uint8 NoteKind; // To be used with ENoteKind.

		struct {
			char FractionKind : 4;
			char EnabledHitFlags : 4;
		};

		NoteData() {
			StartTime = EndTime = 0;
			Sound = 0;
			NoteKind = NK_NORMAL;
		}
	};

	class TrackNote
	{
	private:
		// 24 bytes
		NoteData Data;

		// 8 bytes
		float b_pos;
		float b_pos_holdend;

	public:
		TrackNote();

		void AssignNotedata(const NoteData &Data);

		void AssignPosition(float Position, float endPosition = 0);
		void AssignFraction(double frc); // frc = fraction of a beat
		void Hit();

		void AddTime(double Time);
		void Disable();

		float GetVertical() const;
		float GetVerticalHold() const;
		bool IsHold() const;
		bool IsEnabled() const;
		bool WasNoteHit() const;
		int GetSound() const;
		Mat4 GetMatrix() const;
		Mat4 GetHoldPositionMatrix(const float &trackPosition) const;
		Mat4 GetHoldBodyMatrix(const float &noteWidth, const float &speedMultiplier) const;
		Mat4 GetHoldEndMatrix() const;

		double GetTimeFinal() const;
		double GetStartTime() const;

		// These must be calculated when using a non-beat based system. Consider them unreliable.
		double GetBeatStart() const;
		double GetBeatEnd() const;
		int GetFracKind() const;

		~TrackNote();
	};

}
#endif