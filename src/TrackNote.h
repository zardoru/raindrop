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

	glm::vec2 b_pos;
	glm::vec2 b_pos_holdend;

	glm::mat4 final;

	float VerticalHoldBodySize;
	float VerticalHoldBodyPos;

	glm::mat4 hold_body;
	glm::mat4 hold_body_size;
	glm::mat4 hold_final;
public:
	TrackNote();
	void AssignTrack(int T);
	void AssignSongPosition(double _BeatStart, double _BeatEnd = 0);
	void AssignTime(double Start, double End = 0);
	void AssignPosition(glm::vec2 Position, glm::vec2 endPosition = glm::vec2(0,0));
	void Hit();

	void AddTime(double Time);
	void RecalculateBody(float noteWidth, float noteSize, float speedMultiplier);
	void Disable();

	float GetVertical() const;
	float GetVerticalHold() const;
	float GetVerticalSize() const;
	uint32 GetTrack() const;
	bool IsHold() const;
	bool IsEnabled() const;
	bool WasNoteHit() const;

	glm::mat4& GetMatrix();
	glm::mat4& GetHoldBodyMatrix();
	glm::mat4& GetHoldBodySizeMatrix();
	glm::mat4& GetHoldEndMatrix();
	
	double GetTimeFinal() const;
	double GetStartTime() const;
	double GetBeatStart() const;
	double GetBeatEnd() const;

	~TrackNote();
};

#endif