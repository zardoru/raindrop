#ifndef SG7K_H_
#define SG7K_H_

class ScreenGameplay7K : public IScreen
{
private:
	
	/* User Variables */
	float SpeedMultiplierUser;
	bool waveEffectEnabled;	
	bool Upscroll;

	/* Game */
	double CurrentVertical;
	glm::mat4 PositionMatrix;
	glm::mat4 NoteMatrix;
	Song7K *MySong;
	float SongOldTime;
	float deltaPos;
	float SpeedMultiplier;
	bool AudioCompensation;
	double TimeCompensation;
	bool MultiplierChanged;

	/* Positions */
	float  JudgementLinePos;
	float GearLaneWidth, BasePos;

	/* Effects */
	float waveEffect; 
	
	SongInternal::TDifficulty<TrackNote>			 *CurrentDiff;
	std::vector<SongInternal::Measure<TrackNote> >	 NotesByMeasure[16];
	TimingData VSpeeds;
	Image*  NoteImage;
	Image*  NoteImageHold;
	Image*  GearLaneImage[MAX_CHANNELS];
	Image*  GearLaneImageDown[MAX_CHANNELS];
	Image*  NoteImages[MAX_CHANNELS];
	Image*  NoteImagesHold[MAX_CHANNELS];

	int		GearBindings[MAX_CHANNELS];
	uint32	Channels;

	/* Explosions */
	Image*  ExplosionFrames[32];
	GraphObject2D Explosion[MAX_CHANNELS];
	double   ExplosionTime[MAX_CHANNELS];

	PaStreamWrapper *Music;

	GraphObject2D Keys[MAX_CHANNELS];
	GraphObject2D Background;
	GraphObject2D JudgementLine;

	AccuracyData7K Score;
	double SongTime;

	/* 
		Optimizations will come in later. 
		See Renderer7K.cpp.	
	*/

	void RecalculateMatrix();
	void RecalculateEffects();
	void RunMeasures();


	void DrawExplosions();
	void DrawMeasures();

	void JudgeLane(unsigned int Lane);
	void TranslateKey(KeyType K, bool KeyDown);
public:
	ScreenGameplay7K();
	void Init(Song7K *S, int DifficultyIndex, bool UseUpscroll);
	void LoadThreadInitialization();
	void MainThreadInitialization();
	void Cleanup();

	bool Run(double Delta);
	void HandleInput(int32 key, KeyEventType code, bool isMouseInput);
};

#endif
