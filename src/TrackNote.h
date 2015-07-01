

#ifndef TAPNOTE_H_
#define TAPNOTE_H_

namespace VSRG
{
	const unsigned char EnabledFlag = 1 << 0;
	const unsigned char WasHitFlag = 1 << 1;
	const unsigned char HeadEnabledFlag = 1 << 2;

	// REMEMBER WE MUST BE 8-BYTE ALIGNED. It's either 16, 24 or 32...
	// note data: 20 bytes
	// Going to be 24 bytes on its own with alignment...
	struct NoteData
	{
		// start time and end time are 8 bytes each - 16 bytes.
		double StartTime, EndTime;
		uint16 Sound; // Do we really need more than 65536 sounds?
		uint8 NoteKind; // To be used with ENoteKind.

		NoteData() {
			StartTime = EndTime = 0;
			Sound = 0;
			NoteKind = NK_NORMAL;
		}
	};

	class TrackNote
	{
	private:
		// 16 bytes
		double StartTime, EndTime;

		// 8 bytes
		float b_pos;
		float b_pos_holdend;

		// 2 bytes
		uint16 Sound; 

		// 2 bytes
		uint8 NoteKind; // To be used with ENoteKind.

		struct {
			char FractionKind : 4;
			char EnabledHitFlags : 4;
		};

		// With this structure, we'll get 8 spare bytes we can use for whatever.

	public:
		TrackNote();

		void AssignNotedata(const NoteData &Data);
		
		double &GetDataStartTime();
		double &GetDataEndTime();
		uint16 &GetDataSound();
		uint8  GetDataNoteKind();
		uint8  GetDataFractionKind();

		void AssignPosition(float Position, float endPosition = 0);
		void AssignFraction(double frc); // frc = fraction of a beat
		void Hit();

		void AddTime(double Time);
		void Disable();
		void DisableHead();

		float GetVertical() const;
		float GetVerticalHold() const;
		bool IsHold() const;
		bool IsEnabled() const;
		bool IsHeadEnabled() const;
		bool WasNoteHit() const;
		bool IsJudgable() const;
		bool IsVisible() const;
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
		float GetHoldSize();
		float GetHoldEndVertical();
	};

}
#endif
