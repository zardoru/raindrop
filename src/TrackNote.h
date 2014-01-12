#ifndef TAPNOTE_H_
#define TAPNOTE_H_

class TrackNote
{
private:
	
	uint32 Track;
	double StartTime, EndTime;
	double BeatStart, BeatEnd;
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
	void AddTime(double Time);
	void AssignPosition(glm::vec2 Position, glm::vec2 endPosition = glm::vec2(0,0));
	void RecalculateBody(float noteWidth, float noteSize, float speedMultiplier);

	float GetVertical() const;
	float GetVerticalSize() const;
	bool IsHold() const;
	glm::mat4& GetMatrix();
	glm::mat4& GetHoldBodyMatrix();
	glm::mat4& GetHoldBodySizeMatrix();
	glm::mat4& GetHoldMatrix();
	
	double GetTimeFinal() const;
	double GetStartTime() const;
	double GetBeatStart() const;
	double GetBeatEnd() const;

	~TrackNote();
};

#endif