#ifndef TAPNOTE_H_
#define TAPNOTE_H_

class TrackNote
{
private:
	
	uint32 Track;
	float StartTime, EndTime;
	int Measure, Fraction;

public:
	TrackNote();
	void AssignTrack(int T);
	void AssignSongPosition(int _Measure, int _Fraction);
	void AssignTime(float Start, float End);
	~TrackNote();

};

#endif