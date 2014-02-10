#ifndef SG7K_H_
#define SG7K_H_

class GraphObjectMan;
class ScoreKeeper7K;

class ScreenGameplay7K : public IScreen
{
private:
	
	/* User Variables */
	float SpeedMultiplierUser;
	bool waveEffectEnabled;	
	bool Upscroll;

	/* Game */
	bool Active;
	double CurrentVertical;
	Mat4 PositionMatrix;
	Mat4 NoteMatrix[MAX_CHANNELS];
	Song7K *MySong;
	float SongOldTime;
	float deltaPos;
	float SpeedMultiplier;
	bool AudioCompensation;
	double TimeCompensation;
	bool MultiplierChanged;
	SongInternal::TDifficulty<TrackNote>			 *CurrentDiff;
	std::vector<SongInternal::Measure<TrackNote> >	 NotesByMeasure[16];
	TimingData VSpeeds;
	int		GearBindings[MAX_CHANNELS];
	uint32	Channels;
	bool HeldKey[MAX_CHANNELS];

	/* Positions */
	float  JudgementLinePos;
	float  BasePos;

	/* Effects */
	float waveEffect; 
	
	
	/* Graphics */
	Image*  NoteImage;
	Image*  NoteImageHold;
	Image*  GearLaneImage[MAX_CHANNELS];
	Image*  GearLaneImageDown[MAX_CHANNELS];
	Image*  NoteImages[MAX_CHANNELS];
	Image*  NoteImagesHold[MAX_CHANNELS];
	float NoteHeight;
	GraphObject2D Keys[MAX_CHANNELS];
	GraphObject2D Background;
	GraphObjectMan *Animations;
	float LanePositions[MAX_CHANNELS];
	float LaneWidth[MAX_CHANNELS];
	float GearHeightFinal;

	AudioStream *Music;

	ScoreKeeper7K* score_keeper;
	double SongTime, SongTimeReal;

	/* 
		Optimizations will come in later. 
		See Renderer7K.cpp.	
	*/

	void SetupGear();

	void SetupScriptConstants();
	void UpdateScriptVariables();

	void RecalculateMatrix();
	void RecalculateEffects();
	void RunMeasures();

	void HitNote (double TimeOff, uint32 Lane);
	void MissNote (double TimeOff, uint32 Lane);

	void DrawMeasures();

	void JudgeLane(unsigned int Lane);
	void ReleaseLane(unsigned int Lane);
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
