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
	int lastClosest[MAX_CHANNELS];
	double CurrentVertical;
	Mat4 PositionMatrix;
	Mat4 NoteMatrix[MAX_CHANNELS];
	Mat4 HoldHeadMatrix[MAX_CHANNELS];
	Mat4 HoldTailMatrix[MAX_CHANNELS];
	Song7K *MySong;
	float SongOldTime;
	float deltaPos;
	float SpeedMultiplier;
	bool AudioCompensation;
	double TimeCompensation;
	bool MultiplierChanged;
	SongInternal::Difficulty7K			 *CurrentDiff;
	std::vector<SongInternal::Measure7K>	 NotesByMeasure[MAX_CHANNELS];
	TimingData VSpeeds;
	int		GearBindings[MAX_CHANNELS];
	uint32	Channels;
	bool HeldKey[MAX_CHANNELS];
	
	float CurrentBeat;

	std::map <int, SoundSample*> Keysounds;
	std::vector<SongInternal::AutoplaySound> BGMEvents;

	int PlaySounds[MAX_CHANNELS];

	/* Positions */
	float  JudgementLinePos;
	float  BasePos;

	/* Effects */
	float waveEffect; 
	float beatScrollEffect;

	bool beatScrollEffectEnabled;
	glm::mat4 noteEffectsMatrix[MAX_CHANNELS];
	
	
	/* Graphics */
	Image*  NoteImage;
	Image*  NoteImageHold;
	Image*  GearLaneImage[MAX_CHANNELS];
	Image*  GearLaneImageDown[MAX_CHANNELS];
	Image*  NoteImages[MAX_CHANNELS];
	Image*  NoteImagesHold[MAX_CHANNELS];
	Image*  NoteImagesHoldHead[MAX_CHANNELS];
	Image*  NoteImagesHoldTail[MAX_CHANNELS];
	double NoteHeight;
	double HoldHeadHeight;
	double HoldTailHeight;
	GraphObject2D Keys[MAX_CHANNELS];
	GraphObject2D Background;
	GraphObjectMan *Animations;
	double LanePositions[MAX_CHANNELS];
	double LaneWidth[MAX_CHANNELS];
	double GearHeightFinal;

	AudioStream *Music;
	SoundSample *MissSnd;

	ScoreKeeper7K* score_keeper;
	double SongTime, SongTimeReal;

	bool InterpolateTime;
	double ErrorTolerance;
	/* 
		Optimizations will come in later. 
		See Renderer7K.cpp.	
	*/

	void SetupGear();

	void SetupScriptConstants();
	void UpdateScriptVariables();
	void UpdateScriptScoreVariables();

	void RecalculateMatrix();
	void RecalculateEffects();
	void RunMeasures();

	void HitNote (double TimeOff, uint32 Lane, bool IsHold, bool IsHoldRelease = false);
	void MissNote (double TimeOff, uint32 Lane, bool IsHold, bool auto_hold_miss);

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
