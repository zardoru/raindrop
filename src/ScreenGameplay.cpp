#include "Global.h"
#include <boost/format.hpp>
#include "Game_Consts.h"
#include "ScreenGameplay.h"
#include "ScreenEvaluation.h"
#include "NoteLoader.h"
#include "GraphicsManager.h"
#include "ImageLoader.h"
#include "Audio.h"

cAudio::IAudioSource* song;

ScreenGameplay::ScreenGameplay(IScreen *Parent) :
	IScreen(Parent),
	Barline(this)
{
	Running = true;
	IsAutoplaying = false;
	ShouldChangeScreenAtEnd = true;
}

glm::vec2 ScreenGameplay::GetScreenOffset(float Alignment)
{
	float outx; 
	float outy;

	outx = (GraphMan.GetMatrixSize().x*Alignment);
	outy = (GraphMan.GetMatrixSize().y*Alignment);

	return glm::vec2(outx, outy);
}

void ScreenGameplay::StoreEvaluation(Judgement Eval)
{
	if (Eval > Bad || Eval == OK)
		Combo++;

	switch (Eval)
	{
	case Excellent:
		Evaluation.NumExcellents++;
		break;
	case Perfect:
		Evaluation.NumPerfects++;
		break;
	case Great:
		Evaluation.NumGreats++;
		break;
	case Bad:
		Evaluation.NumBads++;
		Combo = 0;
		break;
	case Miss:
		Evaluation.NumMisses++;
		Combo = 0;
		break;
	case NG:
		Evaluation.NumNG++;
		Combo = 0;
		break;
	case OK:
		Evaluation.NumOK++;
		break;
	case None:
		break;
	}
	Evaluation.MaxCombo = max(Evaluation.MaxCombo, Combo);
}

void ScreenGameplay::Init(Song *OtherSong)
{
	MySong = OtherSong;
	
	memset(&Evaluation, 0, sizeof(Evaluation));

	Measure = 0;
	Combo = 0;

	Cursor.setImage(ImageLoader::LoadSkin("cursor.png"));
	Barline.setImage(ImageLoader::LoadSkin("Barline.png"));
	MarkerA.setImage(ImageLoader::LoadSkin("barline_marker.png"));
	MarkerB.setImage(ImageLoader::LoadSkin("barline_marker.png"));
	Background.setImage(ImageLoader::Load(MySong->BackgroundDir));

	Background.alpha = 0.8;
	Background.width = PlayfieldWidth;
	Background.height = PlayfieldHeight;
	Background.position.x = GetScreenOffset(0.5).x - Background.width / 2;
	Background.position.y = ScreenOffset;
	Background.Init();

	MarkerA.origin = MarkerB.origin = 1;
	MarkerA.position.x = MarkerB.position.x = GetScreenOffset(0.5).x;
	MarkerA.position.y = MarkerA.height / 2 + ScreenOffset;
	MarkerB.position.y = PlayfieldHeight - MarkerB.height / 2 + ScreenOffset;
	MarkerB.rotation = 180;
	MarkerA.width = MarkerB.width = Lifebar.width = PlayfieldWidth;
	MarkerA.Init();
	MarkerB.Init();

	Barline.Init(MySong->Offset);
	Lifebar.UpdateHealth();

	MeasureTime = (60 * 4 / MySong->BPM);

	NotesInMeasure.resize(MySong->MeasureCount);
	for (int i = 0; i < MySong->MeasureCount; i++)
	{
		NotesInMeasure[i] = MySong->GetObjectsForMeasure(i);
	}

	song = audioMgr->create("song", MySong->SongDir.c_str(), true);

	if (!song)
		throw std::exception( (boost::format ("couldn't open song %s") % MySong->SongDir).str().c_str() );

	MeasureTimeElapsed = 0;
	SongTime = 0;
	ScreenTime = 0;

	// We might be retrying- in that case we should probably clean up.
	if (NotesHeld.size() > 0)
		NotesHeld.clear();

	if (AnimateOnly.size() > 0)
		AnimateOnly.clear();
	Cursor.Init();
}

int ScreenGameplay::GetMeasure()
{
	return Measure;
}

/* Note stuff */

void ScreenGameplay::RunMeasure(float delta)
{
	if (Measure < MySong->MeasureCount)
	{
		Judgement Val;

		// For each note in current measure
		for (std::vector<GameObject>::iterator i = NotesInMeasure[Measure].begin(); 
			i != NotesInMeasure[Measure].end(); 
			i++)
		{
			// Run the note.
			if ((Val = i->Run(delta, SongTime, IsAutoplaying)) != None)
			{
				Lifebar.HitJudgement(Val);
				aJudgement.ChangeJudgement(Val);
				StoreEvaluation(Val);
				break;
			}
		}
	}

	if (NotesHeld.size() > 0)
	{
		Judgement Val;
		for (std::vector<GameObject>::iterator i = NotesHeld.begin();
			i != NotesHeld.end();
			i++)
		{
			// See if something's going on with the hold.
			if ((Val = i->Run(delta, SongTime, IsAutoplaying)) != None)
			{
				// Judge accordingly..
				Lifebar.HitJudgement(Val);
				aJudgement.ChangeJudgement(Val);
				StoreEvaluation(Val);
				AnimateOnly.push_back(*i); // Animate this one.
				i = NotesHeld.erase(i);
				break;
			}
		}
	}
}

void ScreenGameplay::HandleInput(int key, int code, bool isMouseInput)
{
	glm::vec2 mpos = GraphMan.GetRelativeMPos();

	if (Next && Next->IsScreenRunning())
	{
		Next->HandleInput(key, code, isMouseInput);
		return;
	}

	if (Measure < MySong->MeasureCount && // if measure is playable
		(((key == 'Z' || key == 'X') && !isMouseInput) || // key is z or x and it's not mouse input or
		(isMouseInput && (key == GLFW_MOUSE_BUTTON_LEFT || key == GLFW_MOUSE_BUTTON_RIGHT))) // is mouse input and it's a mouse button..
		)
	{
		Judgement Val;

		// For all measure notes..
		for (std::vector<GameObject>::iterator i = NotesInMeasure[Measure].begin(); 
			i != NotesInMeasure[Measure].end(); 
			i++)
		{
			// See if it's a hit.
			if ((Val = i->Hit(SongTime, mpos, code == GLFW_PRESS ? true : false, IsAutoplaying, key)) != None)
			{
				// Judge accordingly.
				Lifebar.HitJudgement(Val);
				aJudgement.ChangeJudgement(Val);
				StoreEvaluation(Val);

				// If it's a hold, keep running it until it's done.
				if (i->endTime && Val > Miss)
				{
					NotesHeld.push_back(*i);
					i = NotesInMeasure[Measure].erase(i); // These notes are off the measure. We'll handle them somewhere else.
				}
				break;
			}
		}

		// For all held notes...
		for (std::vector<GameObject>::iterator i = NotesHeld.begin();
			i != NotesHeld.end();
			i++)
		{
			// See if something's going on with the hold.
			if ((Val = i->Hit(SongTime, mpos, code == GLFW_PRESS ? true : false, IsAutoplaying, key)) != None)
			{
				// Judge accordingly..
				Lifebar.HitJudgement(Val);
				aJudgement.ChangeJudgement(Val);
				StoreEvaluation(Val);
				i = NotesHeld.erase(i); // Delete this object. The hold is done!
				break;
			}
		}
		return;
	}

	if (code == GLFW_PRESS)
	{
#ifndef NDEBUG
		if (key == 'R') // Retry
		{
			Cleanup();
			Init(MySong);
			return;
		}
#endif

		if (key == 'A') // Autoplay
		{
			IsAutoplaying = !IsAutoplaying;
		}

		if (key == GLFW_KEY_ESC)
		{
			Running = false;
			Cleanup();
		}
	}
}

void ScreenGameplay::Cleanup()
{
	// Deleting the song's notes is ScreenSelectMusic's (or fileman's) job.
	if (song != NULL)
	{
		song->stop();
		song->drop();
		song = NULL;
	}
}

void ScreenGameplay::seekTime(float Time)
{
	song->seek(Time);
	SongTime = Time;
	ScreenTime = 0;
}

void ScreenGameplay::startMusic()
{
	song->play();
}

void ScreenGameplay::stopMusic()
{
	song->stop();
}

// todo: important- use song's time instead of counting manually.
bool ScreenGameplay::Run(float TimeDelta)
{
	ScreenTime += TimeDelta;
	if (ScreenTime > ScreenPauseTime) // we're over the pause?
	{
		if (SongTime == 0)
		{
			song->seek(TimeDelta, true);
			song->play();
		}

		SongTime += TimeDelta;

		if (Next) // We have a pending screen?
		{
			return RunNested(TimeDelta); // use that instead.
		}

		// do we need this call?
		// glfwPollEvents();

		glm::vec2 mpos = GraphMan.GetRelativeMPos();

		RunMeasure(TimeDelta);

		Cursor.position = mpos;

		Barline.Run(TimeDelta, MeasureTime, MeasureTimeElapsed);

		if (SongTime > MySong->Offset)
		{
			MeasureTimeElapsed += TimeDelta;
			if (MeasureTimeElapsed > MeasureTime)
			{
				MeasureTimeElapsed -= MeasureTime;
				Measure += 1;
			}
			Lifebar.Run(TimeDelta);
			aJudgement.Run(TimeDelta);
		}

		RenderObjects(TimeDelta);
	}

	if (Lifebar.Health <= 0 && ShouldChangeScreenAtEnd) // You died? Not an infinite screen?
		Running = false; // gg

	return Running;
}

void ScreenGameplay::RenderObjects(float TimeDelta)
{
	// Rendering ahead.
	Background.Render();
	MarkerA.Render();
	MarkerB.Render();
	Lifebar.Render();

	if (NotesHeld.size() > 0)
	{
		for (std::vector<GameObject>::reverse_iterator i = NotesHeld.rbegin(); 
			i != NotesHeld.rend(); 
			i++)
		{
			i->Animate(TimeDelta, SongTime);
			i->Render();
		}
	}

	if (AnimateOnly.size() > 0)
	{
		for (std::vector<GameObject>::reverse_iterator i = AnimateOnly.rbegin(); 
			i != AnimateOnly.rend(); 
			i++)
		{
			i->Animate(TimeDelta, SongTime);
			i->Render();
		}
	}

	if (Measure > 0)
	{
		for (std::vector<GameObject>::reverse_iterator i = NotesInMeasure[Measure-1].rbegin(); 
			i != NotesInMeasure[Measure-1].rend(); 
			i++)
		{
			i->Animate(TimeDelta, SongTime);
			i->Render();
		}
	}

	// Render current measure on front of the next!
	if (Measure + 1 < MySong->MeasureCount)
	{
		// Draw from latest to earliest
		for (std::vector<GameObject>::reverse_iterator i = NotesInMeasure[Measure+1].rbegin(); 
			i != NotesInMeasure[Measure+1].rend(); 
			i++)
		{
			i->Animate(TimeDelta, SongTime);
			i->Render();
		}
	}

	if (Measure < MySong->MeasureCount)
	{
		for (std::vector<GameObject>::reverse_iterator i = NotesInMeasure[Measure].rbegin(); 
			i != NotesInMeasure[Measure].rend(); 
			i++)
		{
			i->Animate(TimeDelta, SongTime);
			i->Render();
		}
	}else
	{
		if (ShouldChangeScreenAtEnd)
		{
			ScreenEvaluation *Eval = new ScreenEvaluation(this);
			Eval->Init(Evaluation);
			Next = Eval;
			song->stop();
		}
	}

	Barline.Render();
	aJudgement.Render();
	Cursor.Render();
}