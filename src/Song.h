#ifndef SONG_H_
#define SONG_H_

#include "GameObject.h"

float spb(float bpm);

class Song 
{
public:
	Song();
	float Offset, BPM;
	int MeasureCount;
	std::string SongDir, BackgroundDir;
	std::string SongName;
	std::vector<GameObject> *Notes; // todo: wrap this within an individual chart
	void Process();
	std::vector<GameObject> GetObjectsForMeasure(int measure);
};

#endif