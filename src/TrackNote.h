#ifndef TAPNOTE_H_
#define TAPNOTE_H_

namespace VSRG
{

	struct NoteData
	{
		double StartTime, EndTime;
		uint32 Sound;
		uint32 FractionKind;

		NoteData() {
			StartTime = EndTime = 0;
			Sound = 0;
			FractionKind = 0;
		}
	};

	class TrackNote
	{
	private:

		NoteData Data;

		bool Enabled;
		bool WasHit;

		Vec2 b_pos;
		Vec2 b_pos_holdend;

		Mat4 final;

		float VerticalHoldBodySize;
		float VerticalHoldBodyPos;

		Mat4 hold_body;
		Mat4 hold_body_size;
		Mat4 hold_final;
	public:
		TrackNote();

		void AssignNotedata(const NoteData &Data);

		void AssignPosition(Vec2 Position, Vec2 endPosition = Vec2(0,0));
		void AssignFraction (double frc); // frc = fraction of a beat
		void Hit();

		void AddTime(double Time);
		void RecalculateBody(float trackPosition, float noteWidth, float noteSize, float speedMultiplier);
		void Disable();

		float GetVertical() const;
		float GetVerticalHold() const;
		float GetVerticalSize() const;
		bool IsHold() const;
		bool IsEnabled() const;
		bool WasNoteHit() const;
		int GetSound() const;
		Mat4& GetMatrix();
		Mat4& GetHoldBodyMatrix();
		Mat4& GetHoldBodySizeMatrix();
		Mat4& GetHoldEndMatrix();

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