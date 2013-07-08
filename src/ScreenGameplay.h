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

	Song *MySong;
	SongInternal::Difficulty* CurrentDiff;
	// Game Data
	uint32 Measure;

	// the point of this is that we can change the barline's position.
	float MeasureTimeElapsed;

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

	std::vector <std::vector<GameObject>> NotesInMeasure;
private: // shit only screengameplay needs

	// Run the current measure (jic func name isn't obvious enough)
	void RunMeasure(float delta);
	void StoreEvaluation(Judgement Eval);
	uint32 Combo;
	BitmapFont MyFont;
	BitmapFont SongInfo;

	// Notes
	std::vector<GameObject> NotesHeld, AnimateOnly;

	// Game Data
	// MeasureTime is the time the current measure will last.
	// MeasureTimeElapsed is the time so far of the current measure
	double MeasureTime;

	// Actors
	ActorLifebar Lifebar;
	ActorJudgement aJudgement;
	GraphObject2D MarkerA, MarkerB, Cursor, Background;
	EvaluationData Evaluation;
	
	double SongTime, SongDelta, SongTimeLatency;
	
	bool FailEnabled;
	bool TappingMode;
	SoundStream *Music;

	void DrawVector(std::vector<GameObject>& Vec, float TimeDelta);
	bool JudgeVector(std::vector<GameObject>& Vec, int code, int key);
	void RunVector(std::vector<GameObject>& Vec, float TimeDelta);
public:
	ScreenGameplay(IScreen *Parent);

	virtual void Init(Song *OtherSong, uint32 DifficultyIndex);
	int32 GetMeasure();
	virtual bool Run(double Delta);
	virtual void HandleInput(int32 key, int32 code, bool isMouseInput);
	glm::vec2 GetScreenOffset(float alignment);
	virtual void Cleanup();
	void RemoveTrash();
};

#endif