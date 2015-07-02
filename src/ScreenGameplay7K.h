#ifndef SG7K_H_
#define SG7K_H_

#include "Song7K.h"
#include "ScoreKeeper.h"
#include "ScreenGameplay7K_Mechanics.h"
#include "BackgroundAnimation.h"

class AudioStream;
class Image;
class SceneEnvironment;
class ScoreKeeper7K;
class Line;
class AudioSourceOJM;
class VSRGMechanics;
class SoundSample;
class LuaManager;

class ScreenGameplay7K : public Screen
{

private:

	VSRGMechanics *MechanicsSet;

	TimingData       VSpeeds;
	TimingData		 BPS;
	TimingData		 Warps;
	VSRG::VectorTN   NotesByChannel;
	std::map <int, SoundSample*> Keysounds;
	vector<AutoplaySound>   BGMEvents;
	vector<float>			 MeasureBarlines;

	VSRG::Difficulty				*CurrentDiff;
	shared_ptr<VSRG::Song>			 MySong;
	shared_ptr<VSRG::Song>			 LoadedSong;
	TimingType						 UsedTimingType;

	shared_ptr<Line> Barline;

	double BarlineOffset;
	double BarlineX;
	double BarlineWidth;
	double NoteHeight;
	double LanePositions[VSRG::MAX_CHANNELS];
	double LaneWidth[VSRG::MAX_CHANNELS];
	double GearHeightFinal;
	double SongTime, SongTimeReal;
	double SongOldTime;
	double WarpedSongTime;
	double CurrentVertical;
	double WaitingTime;
	double ErrorTolerance;
	double TimeCompensation;
	double GameTime;
	double MissTime;
	double FailureTime;
	double SuccessTime;
	double MsDisplayMargin;
	double Speed;

	/* User Variables */
    float       SpeedMultiplierUser;

    EHiddenMode SelectedHiddenMode;

    Mat4             PositionMatrix;

    float            SpeedMultiplier;
    uint32           StartMeasure;

    int              GearBindings[VSRG::MAX_CHANNELS];
	int              GearIsPressed[VSRG::MAX_CHANNELS];
    int              lastClosest[VSRG::MAX_CHANNELS];
    int              PlaySounds[VSRG::MAX_CHANNELS];
    int              BarlineOffsetKind;

	LifeType         lifebar_type;		 
	ScoreType        scoring_type;

	AudioStream *Music;
	AudioSourceOJM *OJMAudio;
	SoundSample *MissSnd;
	SoundSample *FailSnd;

	shared_ptr<ScoreKeeper7K> ScoreKeeper;

	EHiddenMode		 RealHiddenMode;
	float            HideClampLow, HideClampHigh, HideClampFactor;
	float            HideClampSum;

	/* Positions */
	float  JudgmentLinePos;

	/* Effects */
	float waveEffect;
	float beatScrollEffect;

	Mat4 noteEffectsMatrix[VSRG::MAX_CHANNELS];

	float  CurrentBeat;

	bool stage_failed;
	bool beatScrollEffectEnabled;
	bool waveEffectEnabled;
	bool Auto;
	bool perfect_auto; // if enabled, auto will play perfectly.
	bool Upscroll;
	bool NoFail;
	bool Active;
	bool ForceActivation;
	bool DoPlay;
	bool Preloaded;
	bool PlayReactiveSounds;
	bool BarlineEnabled;
	bool SongFinished;

	bool             HeldKey[VSRG::MAX_CHANNELS];
	bool			 MultiplierChanged;

	bool    InterpolateTime;
	bool    AudioCompensation;
	shared_ptr<BackgroundAnimation> BGA;
	/*
		Optimizations will come in later.
		See Renderer7K.cpp.
	*/

	void SetupScriptConstants();
	void SetupLua(LuaManager* Env);
	void SetupMechanics();
	void UpdateScriptVariables();
	void UpdateScriptScoreVariables();
	void CalculateHiddenConstants();

	void ChangeNoteTimeToBeats();

	// Done in loading thread
	bool LoadChartData();
	bool LoadSongAudio();
	bool LoadBGA();
	void SetupBarline();
	bool ProcessSong();

	void SetupAfterLoadingVariables();

	// Done in main thread, after the loading thread
	void SetupBackground();

	void RecalculateMatrix();
	void RecalculateEffects();
	void RunMeasures();

	void HitNote (double TimeOff, uint32 Lane, bool IsHold, bool IsHoldRelease = false);
	void MissNote (double TimeOff, uint32 Lane, bool IsHold, bool auto_hold_miss, bool early_miss);

	void DrawBarlines(float rPos);
	void DrawMeasures();

	void GearKeyEvent(uint32 Lane, bool KeyDown);
	void JudgeLane(uint32 Lane, float Time);
	void ReleaseLane(uint32 Lane, float Time);
	void TranslateKey(KeyType K, bool KeyDown);
	void AssignMeasure(uint32 Measure);
	void RunAutoEvents();
	void CheckShouldEndScreen();
	void UpdateSongTime(float Delta);
	void Render();

	void PlayLaneKeysound(uint32 Lane);
	void PlayKeysound(uint32 Index);
	void SetLaneHoldState(uint32 Lane, bool NewState);

	void Activate();

	// true if holding down key
	bool GetGearLaneState(uint32 Lane);

	friend class Noteskin;
public:

	// Functions for data.
	bool IsAutoEnabled();
	bool IsFailEnabled();
	float GetCurrentBeat();
	float GetUserMultiplier() const;
	float GetCurrentVerticalSpeed();
	float GetCurrentVertical();
	double GetSongTime();
	double GetWarpedSongTime();

	void SetUserMultiplier(float Multip);

	ScreenGameplay7K();
	void Init(shared_ptr<VSRG::Song> S, int DifficultyIndex, const GameParameters &Param);
	void LoadThreadInitialization();
	void MainThreadInitialization();
	void Cleanup();

	bool Run(double Delta);
	bool HandleInput(int32 key, KeyEventType code, bool isMouseInput);
};

#endif
