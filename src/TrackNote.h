#ifndef TAPNOTE_H_
#define TAPNOTE_H_

class TrackNote
{
private:
	
	uint32 Track;
	float StartTime, EndTime;
	int Measure, Fraction;
	glm::mat4 *mt;
	glm::vec2 b_pos;

	glm::mat4 final;
public:
	TrackNote();
	void AssignTrack(int T);
	void AssignSongPosition(int _Measure, int _Fraction);
	void AssignTime(float Start, float End);
	void AssignPosition(glm::vec2 Position);
	void AssignPremultiplyMatrix(glm::mat4* _mat);
	void AssignSpeedMultiplier(float Mult);

	int GetMeasure();
	int GetFraction();
	float GetVertical();
	glm::mat4& GetMatrix();

	~TrackNote();
};

#endif