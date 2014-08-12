#ifndef TAPNOTE_H_
#define TAPNOTE_H_

namespace VSRG
{

	struct NoteData
	{
		double StartTime, EndTime;
		uint32 Sound;

		NoteData() {
			StartTime = EndTime = 0;
			Sound = 0;
		}
	};

	class TrackNote
	{
	private:

		NoteData Data;

		Vec2 b_pos;
		Vec2 b_pos_holdend;
		uint32 FractionKind;

		float VerticalHoldBodySize;
		Mat4 hold_body_size;

		bool Enabled;
		bool WasHit;

	public:
		TrackNote();

		void AssignNotedata(const NoteData &Data);

		void AssignPosition(Vec2 Position, Vec2 endPosition = Vec2(0,0));
		void AssignFraction (double frc); // frc = fraction of a beat
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