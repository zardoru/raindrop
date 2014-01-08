#ifndef TAPNOTE_H_
#define TAPNOTE_H_

class TrackNote
{
private:
	
	uint32 Track;
	double StartTime, EndTime;
	double BeatStart, BeatEnd;
	glm::vec2 b_pos;

	glm::mat4 final;
public:
	TrackNote();
	void AssignTrack(int T);
	void AssignSongPosition(double _BeatStart, double _BeatEnd = 0);
	void AssignTime(double Start, double End = 0);
	void AddTime(double Time);
	void AssignPosition(glm::vec2 Position);

	float GetVertical() const;
	glm::mat4& GetMatrix();
	
	double GetTimeFinal() const;
	double GetStartTime() const;
	double GetBeatStart() const;
	double GetBeatEnd() const;

	~TrackNote();
};

#endif