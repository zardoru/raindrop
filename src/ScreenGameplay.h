#ifndef SCREEN_GP_H_
#define SCREEN_GP_H_

#include <vector>
#include "Screen.h"
#include "Song.h"
#include "ActorBarline.h"
#include "ActorJudgement.h"
#include "ActorLifebar.h"

class ScreenGameplay : public IScreen
{
protected: // shit the edit screen needs

	Song MySong;

	// Game Data
	int Measure;

	bool IsAutoplaying; // true for autoplaying notes

	bool ShouldChangeScreenAtEnd;

	void RenderObjects(float TimeDelta);

	// Two seconds of pause before actually starting the screen.
	static const unsigned int ScreenPauseTime = 2; 
	
	// leave this to who will use it
	void startMusic();
	void seekTime(float time);

private: // shit only screengameplay needs

	// Run the current measure (jic func name isn't obvious enough)
	void RunMeasure(float delta);
	void StoreEvaluation(Judgement Eval);
	unsigned int Combo;

	// Notes
	std::vector <std::vector<GameObject>> NotesInMeasure;
	std::vector<GameObject> NotesHeld, AnimateOnly;

	// Game Data
	// MeasureTime is the time the current measure will last.
	// MeasureTimeElapsed is the time so far of the current measure
	float MeasureTime, MeasureTimeElapsed;

	// Actors
	ActorBarline Barline;
	ActorLifebar Lifebar;
	ActorJudgement aJudgement;
	GraphObject2D MarkerA, MarkerB, Cursor, Background;
	EvaluationData Evaluation;
	
	float SongTime;

public:
	ScreenGameplay(IScreen *Parent);

	virtual void Init(const Song &OtherSong);
	int GetMeasure();
	virtual bool Run(float Delta);
	virtual void HandleInput(int key, int code, bool isMouseInput);
	glm::vec2 GetScreenOffset(float alignment);
	virtual void Cleanup();
};

#endif