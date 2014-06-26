#ifndef SG7K_H_
#define SG7K_H_

#include "Song7K.h"

class GraphObjectMan;
class ScoreKeeper7K;

class ScreenGameplay7K : public Screen
{
private:
	
	/* User Variables */
	float SpeedMultiplierUser;
	bool waveEffectEnabled;	
	bool Upscroll;
	bool NoFail;
	EHiddenMode SelectedHiddenMode;


	/* Game */
	bool Active;
	bool DoPlay;
	int lastClosest[VSRG::MAX_CHANNELS];
	double CurrentVertical;
	double WaitingTime;
	EHiddenMode RealHiddenMode;
	Mat4 PositionMatrix;
	Mat4 PositionMatrixJudgement;
	Mat4 NoteMatrix[VSRG::MAX_CHANNELS];
	Mat4 HoldHeadMatrix[VSRG::MAX_CHANNELS];
	Mat4 HoldTailMatrix[VSRG::MAX_CHANNELS];
	VSRG::Song *MySong;
	float SongOldTime;
	float deltaPos;
	float SpeedMultiplier;
	bool AudioCompensation;
	double TimeCompensation;
	bool MultiplierChanged;
	VSRG::Difficulty			 *CurrentDiff;
	VSRG::VectorTN NotesByChannel;
	TimingData VSpeeds;
	int		GearBindings[VSRG::MAX_CHANNELS];
	uint32	Channels;
	bool HeldKey[VSRG::MAX_CHANNELS];
	float HideClampLow, HideClampHigh, HideClampFactor;
	float HideClampSum;

	float CurrentBeat;

	std::map <int, SoundSample*> Keysounds;
	std::vector<AutoplaySound> BGMEvents;

	int PlaySounds[VSRG::MAX_CHANNELS];

	/* Positions */
	float  JudgementLinePos;
	float  BasePos;

	/* Effects */
	float waveEffect; 
	float beatScrollEffect;

	bool beatScrollEffectEnabled;
	Mat4 noteEffectsMatrix[VSRG::MAX_CHANNELS];
	
	
	/* Graphics */
	Image*  NoteImage;
	Image*  NoteImageHold;
	Image*  GearLaneImage[VSRG::MAX_CHANNELS];
	Image*  GearLaneImageDown[VSRG::MAX_CHANNELS];
	Image*  NoteImages[VSRG::MAX_CHANNELS];
	Image*  NoteImagesHold[VSRG::MAX_CHANNELS];
	Image*  NoteImagesHoldHead[VSRG::MAX_CHANNELS];
	Image*  NoteImagesHoldTail[VSRG::MAX_CHANNELS];
	double NoteHeight;
	double HoldHeadHeight;
	double HoldTailHeight;
	GraphObject2D Keys[VSRG::MAX_CHANNELS];
	GraphObject2D Background;
	GraphObjectMan *Animations;
	double LanePositions[VSRG::MAX_CHANNELS];
	double LaneWidth[VSRG::MAX_CHANNELS];
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
	void CalculateHiddenConstants();


	void RecalculateMatrix();
	void RecalculateEffects();
	void RunMeasures();

	void HitNote (double TimeOff, uint32 Lane, bool IsHold, bool IsHoldRelease = false);
	void MissNote (double TimeOff, uint32 Lane, bool IsHold, bool auto_hold_miss);

	void DrawMeasures();

	void JudgeLane(uint32 Lane);
	void ReleaseLane(uint32 Lane);
	void TranslateKey(KeyType K, bool KeyDown);
public:
	ScreenGameplay7K();
	void Init(VSRG::Song *S, int DifficultyIndex, bool UseUpscroll);
	void LoadThreadInitialization();
	void MainThreadInitialization();
	void Cleanup();

	bool Run(double Delta);
	void HandleInput(int32 key, KeyEventType code, bool isMouseInput);
};

#endif
