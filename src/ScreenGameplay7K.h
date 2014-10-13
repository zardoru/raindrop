#ifndef SG7K_H_
#define SG7K_H_

#include "Song7K.h"

class GraphObjectMan;
class ScoreKeeper7K;
class Line;

class ScreenGameplay7K : public Screen
{
private:

	GraphObject2D Keys[VSRG::MAX_CHANNELS];
	GraphObject2D Background;

	// BGA stuff.
	GraphObject2D LayerMiss;
	GraphObject2D Layer1;
	GraphObject2D Layer2;

	TimingData       VSpeeds;
	VSRG::VectorTN   NotesByChannel;
	std::map <int, SoundSample*> Keysounds;
	std::vector<AutoplaySound> BGMEvents;
	std::vector<float> MeasureBarlines;

	VSRG::Difficulty *CurrentDiff;
	VSRG::Song       *MySong;
	VSRG::Song		 *LoadedSong;

	ImageList				 BMPs;
	std::vector<AutoplayBMP> BMPEvents;
	std::vector<AutoplayBMP> BMPEventsMiss;
	std::vector<AutoplayBMP> BMPEventsLayer;
	std::vector<AutoplayBMP> BMPEventsLayer2;
	Line* Barline;

	double BarlineOffset;
	double BarlineX;
	double BarlineWidth;
	double NoteHeight;
	double HoldHeadHeight;
	double HoldTailHeight;
	double LanePositions[VSRG::MAX_CHANNELS];
	double LaneWidth[VSRG::MAX_CHANNELS];
	double GearHeightFinal;
	double SongTime, SongTimeReal;
	double SongOldTime;
	double CurrentVertical;
	double WaitingTime;
	double ErrorTolerance;
	double TimeCompensation;
	double GameTime;
	double MissTime;


	/* User Variables */
	float       SpeedMultiplierUser;

	EHiddenMode SelectedHiddenMode;

	Mat4			 PositionMatrix;
	Mat4			 PositionMatrixJudgment;
	Mat4			 NoteMatrix[VSRG::MAX_CHANNELS];
	Mat4			 HoldHeadMatrix[VSRG::MAX_CHANNELS];
	Mat4			 HoldTailMatrix[VSRG::MAX_CHANNELS];

	float			 SpeedMultiplier;

	uint32	         Channels;
	uint32			 StartMeasure;

	int		         GearBindings[VSRG::MAX_CHANNELS];
	int				 lastClosest[VSRG::MAX_CHANNELS];
	int				 PlaySounds[VSRG::MAX_CHANNELS];
	int				 BarlineOffsetKind;

	/* Graphics */
	Image*  NoteImage;
	Image*  NoteImageHold;
	Image*  GearLaneImage[VSRG::MAX_CHANNELS];
	Image*  GearLaneImageDown[VSRG::MAX_CHANNELS];
	Image*  NoteImages[VSRG::MAX_CHANNELS];
	Image*  NoteImagesHold[VSRG::MAX_CHANNELS];
	Image*  NoteImagesHoldHead[VSRG::MAX_CHANNELS];
	Image*  NoteImagesHoldTail[VSRG::MAX_CHANNELS];

	GraphObjectMan *Animations;

	AudioStream *Music;
	SoundSample *MissSnd;

	ScoreKeeper7K* score_keeper;

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

	bool beatScrollEffectEnabled;
	bool waveEffectEnabled;
	bool Auto;
	bool Upscroll;
	bool NoFail;
	bool Active;
	bool DoPlay;
	bool Preloaded;
	bool PlayReactiveSounds;
	bool BarlineEnabled;
	bool SongFinished;

	bool             HeldKey[VSRG::MAX_CHANNELS];
	bool			 MultiplierChanged;

	bool    InterpolateTime;
	bool    AudioCompensation;

	/*
		Optimizations will come in later.
		See Renderer7K.cpp.
	*/

	void SetupGear();

	void SetupScriptConstants();
	void UpdateScriptVariables();
	void UpdateScriptScoreVariables();
	void CalculateHiddenConstants();

	// Done in loading thread
	bool LoadChartData();
	bool LoadSongAudio();
	bool LoadBMPs();
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
public:

	struct Parameters {
		// If true, use upscroll
		bool Upscroll;

		// If true, enable Wave
		bool Wave;

		// If true, assume difficulty is already loaded and is not just metadata
		bool Preloaded;

		// Fail disabled if true.
		bool NoFail;

		// Auto mode enabled if true.
		bool Auto;

		// Selected hidden mode
		EHiddenMode HiddenMode;

		// Selected starting measure
		uint32 StartMeasure;

		Parameters() {
			Upscroll = false;
			Wave = false;
			Preloaded = false;
			Auto = false;
			HiddenMode = HIDDENMODE_NONE;
			StartMeasure = 0;
		}
	};

	ScreenGameplay7K();
	void Init(VSRG::Song *S, int DifficultyIndex, const Parameters &Param);
	void LoadThreadInitialization();
	void MainThreadInitialization();
	void Cleanup();

	bool Run(double Delta);
	void HandleInput(int32 key, KeyEventType code, bool isMouseInput);
};

#endif
