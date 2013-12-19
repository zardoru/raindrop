#ifndef SG7K_H_
#define SG7K_H_

class ScreenGameplay7K : public IScreen
{
private:
	int Measure;
	float Speed, SpeedMultiplier, SpeedMultiplierUser;
	float SongOldTime;
	float deltaPos;
	double CurrentVertical;
	float GearLaneWidth, BasePos;
	float waveEffect; 
	bool waveEffectEnabled;
	glm::mat4 PositionMatrix;
	Song7K *MySong;
	
	SongInternal::TDifficulty<TrackNote>			 *CurrentDiff;
	std::vector<SongInternal::Measure<TrackNote>>	 NotesByMeasure[16];
	std::vector<SongInternal::TDifficulty<TrackNote>::TimingSegment> VSpeeds;
	Image*  NoteImage;
	uint32	Channels;

	PaStreamWrapper *Music;

	GraphObject2D Keys[16];
	GraphObject2D Background;

	/* 
		Optimizations will come in later. 
		See Renderer7K.cpp.	
	*/
	void RecalculateMatrix();
	void RecalculateEffects();
	void DrawMeasures();

public:
	ScreenGameplay7K();
	void Init(Song7K *S, int DifficultyIndex);
	void LoadThreadInitialization();
	void MainThreadInitialization();
	void Cleanup();

	bool Run(double Delta);
	void HandleInput(int32 key, KeyEventType code, bool isMouseInput);
};

#endif