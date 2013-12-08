#ifndef SCREEN_GP_H_
#define SCREEN_GP_H_

#include <vector>
#include "Screen.h"
#include "Song.h"
#include "Audio.h"
#include "ActorBarline.h"
#include "ActorJudgement.h"
#include "ActorLifebar.h"
#include "BitmapFont.h"

class ScreenGameplay : public IScreen
{
protected: // shit the edit screen needs

	SongDC *MySong;
	SongInternal::TDifficulty<GameObject>* CurrentDiff;
	// Game Data
	uint32 Measure;

	// the point of this is that we can change the barline's position.
	float MeasureRatio, RatioPerSecond;
	std::vector<SongInternal::TDifficulty<GameObject>::TimingSegment> BarlineRatios;

	bool IsAutoplaying; // true for autoplaying notes

	bool ShouldChangeScreenAtEnd;
	bool EditMode;

	void RenderObjects(float TimeDelta);

	// Two seconds of pause before actually starting the screen.
	static const uint32 ScreenPauseTime = 2; 
	
	// leave this to who will use it
	void startMusic();
	void seekTime(float time);
	void stopMusic();

	// actors we need to access on edit
	ActorBarline Barline;

	SoundStream *Music;

	std::vector <std::vector<GameObject>> NotesInMeasure;
	void DrawVector(std::vector<GameObject>& Vec, float TimeDelta);

private: // shit only screengameplay needs

	void ProcessBarlineRatios();
	// Run the current measure (jic func name isn't obvious enough)
	void RunMeasure(float delta);
	void StoreEvaluation(Judgement Eval);
	uint32 Combo;
	BitmapFont MyFont;
	BitmapFont SongInfo;
	GraphObject2D ReadySign;

	// Notes
	std::vector<GameObject> NotesHeld, AnimateOnly;

	// Game Data
	// MeasureTime is the time the current measure will last.
	// MeasureTimeElapsed is the time so far of the current measure
	double MeasureTime;
	double LeadInTime;

	// Actors
	ActorLifebar Lifebar;
	ActorJudgement aJudgement;
	GraphObject2D MarkerA, MarkerB, Cursor, Background;
	EvaluationData Evaluation;
	
	double SongTime, SongDelta;
	
	bool FailEnabled;
	bool TappingMode;
	bool IsPaused;

	bool JudgeVector(std::vector<GameObject>& Vec, int code, int key);
	void RunVector(std::vector<GameObject>& Vec, float TimeDelta);

	Image* GameplayObjectImage;
public:
	ScreenGameplay(IScreen *Parent);

	virtual void Init(SongDC *OtherSong, uint32 DifficultyIndex);

	int32 GetMeasure();
	virtual bool Run(double Delta);
	virtual void HandleInput(int32 key, KeyEventType code, bool isMouseInput);
	glm::vec2 GetScreenOffset(float alignment);
	virtual void Cleanup();
	void RemoveTrash();

	/* What we call from the ScreenLoading thread! */
	void LoadThreadInitialization();
	void MainThreadInitialization();
};

#endif