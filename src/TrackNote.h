#ifndef TAPNOTE_H_
#define TAPNOTE_H_

class TrackNote
{
private:
	
	uint32 Track;
	double StartTime, EndTime;
	double BeatStart, BeatEnd;
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
	void AssignTrack(int T);
	void AssignSongPosition(double _BeatStart, double _BeatEnd = 0);
	void AssignTime(double Start, double End = 0);
	void AssignPosition(Vec2 Position, Vec2 endPosition = Vec2(0,0));
	void Hit();

	void AddTime(double Time);
	void RecalculateBody(float trackPosition, float noteWidth, float noteSize, float speedMultiplier);
	void Disable();

	float GetVertical() const;
	float GetVerticalHold() const;
	float GetVerticalSize() const;
	uint32 GetTrack() const;
	bool IsHold() const;
	bool IsEnabled() const;
	bool WasNoteHit() const;

	Mat4& GetMatrix();
	Mat4& GetHoldBodyMatrix();
	Mat4& GetHoldBodySizeMatrix();
	Mat4& GetHoldEndMatrix();
	
	double GetTimeFinal() const;
	double GetStartTime() const;
	double GetBeatStart() const;
	double GetBeatEnd() const;

	~TrackNote();
};

#endif