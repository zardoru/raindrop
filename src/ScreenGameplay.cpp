#include "Global.h"
#include <boost/format.hpp>
#include "Game_Consts.h"
#include "ScreenGameplay.h"
#include "ScreenEvaluation.h"
#include "NoteLoader.h"
#include "GraphicsManager.h"
#include "ImageLoader.h"
#include "Audio.h"

ScreenGameplay::ScreenGameplay(IScreen *Parent) :
	IScreen(Parent),
	Barline(this)
{
	Running = true;
	IsAutoplaying = false;
	ShouldChangeScreenAtEnd = true;
	MyFont.LoadSkinFontImage("font.tga", glm::vec2(18, 32), glm::vec2(34,34), glm::vec2(40,64), 32);
	Music = NULL;
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
	Evaluation.MaxCombo = std::max(Evaluation.MaxCombo, Combo);
}

void ScreenGameplay::RemoveTrash()
{
	NotesHeld.clear();
	AnimateOnly.clear();
	NotesInMeasure.clear();
}

void ScreenGameplay::Init(Song *OtherSong, uint32 DifficultyIndex)
{
	MySong = OtherSong;
	CurrentDiff = MySong->Difficulties[DifficultyIndex];
	
	memset(&Evaluation, 0, sizeof(Evaluation));

	Measure = 0;
	Combo = 0;

	Cursor.setImage(ImageLoader::LoadSkin("cursor.png"));
	Barline.setImage(ImageLoader::LoadSkin("Barline.png"));
	MarkerA.setImage(ImageLoader::LoadSkin("barline_marker.png"));
	MarkerB.setImage(ImageLoader::LoadSkin("barline_marker.png"));
	Background.setImage(ImageLoader::Load(MySong->BackgroundDir));

	Background.alpha = 0.8f;
	Background.width = ScreenWidth;
	Background.height = ScreenHeight;
	Background.position.x = 0;
	Background.position.y = 0;
	Background.Init();

	MarkerA.Centered = MarkerB.Centered = true;
	MarkerA.position.x = MarkerB.position.x = GetScreenOffset(0.5).x;
	MarkerA.position.y = MarkerA.height/2 - Barline.height/2;
	MarkerA.rotation = 180;
	MarkerB.position.y = PlayfieldHeight + ScreenOffset + MarkerB.height/2 + Barline.height/2;
	// MarkerB.rotation = 180;
	Lifebar.width = PlayfieldWidth;
	MarkerA.Init();
	MarkerB.Init();

	if (ShouldChangeScreenAtEnd)
		Barline.Init(CurrentDiff->Offset);
	else
		Barline.Init(0); // edit mode
	Lifebar.UpdateHealth();

	if (CurrentDiff->Timing.size()) // Not edit mode. XXX
		MeasureTime = (60 * 4 / CurrentDiff->Timing[0].Value);
	else
		MeasureTime = 0;

	// We might be retrying- in that case we should probably clean up.
	RemoveTrash();

	// todo: not need to copy this whole thing. 
	NotesInMeasure.resize(CurrentDiff->Measures.size());
	for (uint32 i = 0; i < CurrentDiff->Measures.size(); i++)
	{
		NotesInMeasure[i] = CurrentDiff->Measures[i].MeasureNotes;
	}

	// xxx : new audio system (portaudio)
	// song = audioMgr->create("song", MySong->SongFilename.c_str(), true);

	/*if (!song)
		throw std::exception( (boost::format ("couldn't open song %s") % MySong->SongFilename).str().c_str() );*/

	if (!Music)
	{
		Music = new SoundStream(MySong->SongFilename.c_str());

		if (!Music || !Music->IsValid())
			throw std::exception( (boost::format ("couldn't open song %s") % MySong->SongFilename).str().c_str() );
	}

	MeasureTimeElapsed = 0;
	SongTime = 0;
	ScreenTime = 0;

	Cursor.Init();
}

int32 ScreenGameplay::GetMeasure()
{
	return Measure;
}

/* Note stuff */

void ScreenGameplay::RunMeasure(float delta)
{
	if (Measure < CurrentDiff->Measures.size())
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

void ScreenGameplay::HandleInput(int32 key, int32 code, bool isMouseInput)
{
	glm::vec2 mpos = GraphMan.GetRelativeMPos();

	if (Next && Next->IsScreenRunning())
	{
		Next->HandleInput(key, code, isMouseInput);
		return;
	}

	if (Measure < CurrentDiff->Measures.size() && // if measure is playable
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
			Init(MySong, 0);
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
	if (Music != NULL)
	{
		delete Music;
		Music = NULL;
	}
}

void ScreenGameplay::seekTime(float Time)
{
	Music->Seek(Time, false);
	SongTime = Time;
	ScreenTime = 0;
}

void ScreenGameplay::startMusic()
{
	Music->Start();
}

void ScreenGameplay::stopMusic()
{
	Music->Stop();
}

// todo: important- use song's time instead of counting manually.
bool ScreenGameplay::Run(double TimeDelta)
{
	ScreenTime += TimeDelta;
	
	if (Music)
	{
		SongDelta = Music->GetPlaybackTime() - SongTime;
		SongTime += SongDelta;
	}

	if ( ScreenTime > ScreenPauseTime || !ShouldChangeScreenAtEnd ) // we're over the pause?
	{
		if (SongTime == 0)
			Music->Start();

		if (Next) // We have a pending screen?
		{
			return RunNested(TimeDelta); // use that instead.
		}

		glm::vec2 mpos = GraphMan.GetRelativeMPos();

		RunMeasure(SongDelta);

		Cursor.position = mpos;

		Barline.Run(SongDelta, MeasureTime, MeasureTimeElapsed);

		if (SongTime > CurrentDiff->Offset)
		{
			MeasureTimeElapsed += SongDelta;

			if (SongDelta == 0)
			{
				if (Music->IsStopped())
					MeasureTimeElapsed += TimeDelta;
			}
			if (MeasureTimeElapsed > MeasureTime)
			{
				MeasureTimeElapsed -= MeasureTime;
				Measure += 1;
			}
			Lifebar.Run(SongDelta);
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

	try
	{
		if (Measure > 0)
		{
			if (NotesInMeasure.at(Measure-1).size() > 0)
			{
				for (std::vector<GameObject>::reverse_iterator i = NotesInMeasure[Measure-1].rbegin(); 
					i != NotesInMeasure[Measure-1].rend(); 
					i++)
				{
					i->Animate(TimeDelta, SongTime);
					i->Render();
				}
			}
		}

		// Render current measure on front of the next!
		if (Measure + 1 < CurrentDiff->Measures.size())
		{
			// Draw from latest to earliest
			if (NotesInMeasure.at(Measure+1).size() > 0)
			{
				for (std::vector<GameObject>::reverse_iterator i = NotesInMeasure[Measure+1].rbegin(); 
					i != NotesInMeasure[Measure+1].rend(); 
					i++)
				{
					i->Animate(TimeDelta, SongTime);
					i->Render();
				}
			}
		}

		if (Measure < CurrentDiff->Measures.size())
		{
			if (NotesInMeasure.at(Measure).size() > 0)

			{
				for (std::vector<GameObject>::reverse_iterator i = NotesInMeasure[Measure].rbegin(); 
					i != NotesInMeasure[Measure].rend(); 
					i++)
				{
					i->Animate(TimeDelta, SongTime);
					i->Render();
				}
			}
		}else
		{
			if (ShouldChangeScreenAtEnd)
			{
				ScreenEvaluation *Eval = new ScreenEvaluation(this);
				Eval->Init(Evaluation);
				Next = Eval;
				Music->Stop();
			}
		}
	}catch (...)
	{
	}

	Barline.Render();
	aJudgement.Render();
	std::stringstream str;
	str << Combo;
	MyFont.DisplayText(str.str().c_str(), glm::vec2(aJudgement.position.x, 0));
	Cursor.Render();
}