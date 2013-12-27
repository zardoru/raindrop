#ifndef TAPNOTE_H_
#define TAPNOTE_H_

class TrackNote
{
private:
	
	uint32 Track;
	double StartTime, EndTime;
	int Measure, Fraction;
	glm::vec2 b_pos;

	glm::mat4 final;
public:
	TrackNote();
	void AssignTrack(int T);
	void AssignSongPosition(int _Measure, int _Fraction);
	void AssignTime(double Start, double End);
	void AssignPosition(glm::vec2 Position);
	void AssignSpeedMultiplier(float Mult);

	int GetMeasure() const;
	int GetFraction() const;
	float GetVertical() const;
	glm::mat4& GetMatrix();
	
	double GetTimeFinal() const;
	double GetStartTime() const;

	~TrackNote();
};

#endif