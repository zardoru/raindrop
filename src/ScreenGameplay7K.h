#pragma once

#include "Audio.h"
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
class LuaManager;

class ScreenGameplay7K : public Screen
{

private:

	shared_ptr<VSRGMechanics> MechanicsSet;

	bool HasNegativeScroll;
	TimingData         VSpeeds;
	TimingData		   BPS;
	TimingData		   Warps;
	VSRG::VectorSpeeds Speeds;
	VSRG::VectorTN  NotesByChannel;
	std::map <int, vector<shared_ptr<SoundSample>> > Keysounds;
	std::queue<AutoplaySound>   BGMEvents;
	vector<float>			 MeasureBarlines;

	shared_ptr<VSRG::Difficulty>	 CurrentDiff;
	shared_ptr<VSRG::Song>			 MySong;
	shared_ptr<VSRG::Song>			 LoadedSong;
	TimingType						 UsedTimingType;

	shared_ptr<Line> Barline;

	double BarlineOffset;
	double NoteHeight;
	double LanePositions[VSRG::MAX_CHANNELS];
	double LaneWidth[VSRG::MAX_CHANNELS];
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
	double AudioStart, AudioOldTime;

	/* User Variables */
	float       SpeedMultiplierUser;

	EHiddenMode SelectedHiddenMode;

	Mat4             PositionMatrix;

	float            SpeedMultiplier;
	int				 StartMeasure;

	map<int, int> GearBindings;
	int                lastClosest[VSRG::MAX_CHANNELS];
	VSRG::TrackNote*   CurrentKeysounds[VSRG::MAX_CHANNELS];
	int                BarlineOffsetKind;

	LifeType         lifebar_type;		 
	ScoreType        scoring_type;

	shared_ptr<AudioStream> Music;
	shared_ptr<AudioSourceOJM> OJMAudio;
	SoundSample MissSnd;
	SoundSample FailSnd;

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

	bool GearIsPressed[VSRG::MAX_CHANNELS];
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
	bool SongFinished;

	bool HeldKey[VSRG::MAX_CHANNELS];
	bool MultiplierChanged;

	bool    InterpolateTime;
	bool    AudioCompensation;
	shared_ptr<BackgroundAnimation> BGA;
	int Random;
	bool TurntableEnabled;
	float JudgeOffset;
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

	void RecalculateMatrix();
	void RecalculateEffects();
	void RunMeasures();

	void HitNote (double TimeOff, uint32 Lane, bool IsHold, bool IsHoldRelease = false);
	void MissNote (double TimeOff, uint32 Lane, bool IsHold, bool auto_hold_miss, bool early_miss);

	void DrawBarlines();
	void DrawMeasures();

	void GearKeyEvent(uint32 Lane, bool KeyDown);
	void JudgeLane(uint32 Lane, float Time);
	void ReleaseLane(uint32 Lane, float Time);
	void TranslateKey(int32 K, bool KeyDown);
	void AssignMeasure(uint32 Measure);
	void RunAutoEvents();
	void CheckShouldEndScreen();
	void UpdateSongTime(float Delta);
	void Render();

	void PlayLaneKeysound(uint32 Lane);
	void PlayKeysound(VSRG::TrackNote* Note);
	void SetLaneHoldState(uint32 Lane, bool NewState);

	void Activate();

	// true if holding down key
	bool GetGearLaneState(uint32 Lane);

	friend class Noteskin;
public:

	// Functions for data.
	bool IsAutoEnabled();
	bool IsFailEnabled();
	bool IsUpscrolling();
	float GetCurrentBeat();
	float GetUserMultiplier() const;
	float GetCurrentVerticalSpeed();
	float GetCurrentVertical();
	double GetSongTime();
	double GetWarpedSongTime();

	void SetUserMultiplier(float Multip);

	ScreenGameplay7K();
	void Init(shared_ptr<VSRG::Song> S, int DifficultyIndex, const GameParameters &Param);
	void LoadResources();
	bool BindKeysToLanes(bool UseTurntable);
	void InitializeResources();
	void Cleanup();

	bool Run(double Delta);
	bool HandleInput(int32 key, KeyEventType code, bool isMouseInput);
};
