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
	SongInfo.LoadSkinFontImage("font.tga", glm::vec2(18, 32), glm::vec2(34,34), glm::vec2(10,16), 32);
	MyFont.LoadSkinFontImage("font.tga", glm::vec2(18, 32), glm::vec2(34,34), glm::vec2(40,64), 32);
	Music = NULL;
	FailEnabled = true;
	TappingMode = false;
	EditMode = false;
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

	MarkerB.Centered = true;
	MarkerA.Centered = false;

	MarkerA.position.x = MarkerB.position.x = GetScreenOffset(0.5).x;
	MarkerA.rotation = 180;

	MarkerB.position.y = PlayfieldHeight + ScreenOffset + MarkerB.height/2 + Barline.height/2;
	MarkerA.position.x = 2*MarkerA.position.x;
	MarkerA.position.y = MarkerA.height;
	Lifebar.width = PlayfieldWidth;
	MarkerA.InitTexture();
	MarkerB.InitTexture();

	if (ShouldChangeScreenAtEnd)
		Barline.Init(CurrentDiff->Offset);
	else
	{
		Barline.Init(0); // edit mode
		Barline.alpha = 1;
	}

	
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

	if (!Music)
	{
		Music = new VorbisStream(MySong->SongFilename.c_str());

		if (!Music || !Music->IsOpen())
			throw std::exception( (boost::format ("couldn't open song %s") % MySong->SongFilename).str().c_str() );
		
		MixerAddStream(Music);
		seekTime(0);
	}

	MeasureTimeElapsed = 0;
	ScreenTime = 0;

	Cursor.width = Cursor.height = 72;
	Cursor.Centered = true;
	Cursor.InitTexture();
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

		// For each note in PREVIOUS measure (when not in edit mode)
		if (Measure > 0 && !EditMode)
		{
			for (std::vector<GameObject>::iterator i = NotesInMeasure[Measure-1].begin(); 
				i != NotesInMeasure[Measure-1].end(); 
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


		// Run the ones in the NEXT measure.
		if (Measure+1 < NotesInMeasure.size())
		{
			for (std::vector<GameObject>::iterator i = NotesInMeasure[Measure+1].begin(); 
				i != NotesInMeasure[Measure+1].end(); 
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

		if (code == GLFW_PRESS)
			Cursor.scaleX = Cursor.scaleY = 0.85;
		else
			Cursor.scaleX = Cursor.scaleY = 1;

		// For all measure notes..
		for (std::vector<GameObject>::iterator i = NotesInMeasure[Measure].begin(); 
			i != NotesInMeasure[Measure].end(); 
			i++)
		{
			// See if it's a hit.
			if ((Val = i->Hit(SongTime, TappingMode ? i->position : mpos, code == GLFW_PRESS ? true : false, IsAutoplaying, key)) != None)
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

				if (TappingMode)
					return;
				else
					break;
			}
		}

		if (Measure > 0)
		for (std::vector<GameObject>::iterator i = NotesInMeasure[Measure-1].begin(); 
			i != NotesInMeasure[Measure-1].end(); 
			i++)
		{
			// See if it's a hit.
			if ((Val = i->Hit(SongTime, TappingMode ? i->position : mpos, code == GLFW_PRESS ? true : false, IsAutoplaying, key)) != None)
			{
				// Judge accordingly.
				Lifebar.HitJudgement(Val);
				aJudgement.ChangeJudgement(Val);
				StoreEvaluation(Val);

				// If it's a hold, keep running it until it's done.
				if (i->endTime && Val > Miss)
				{
					NotesHeld.push_back(*i);
					i = NotesInMeasure[Measure-1].erase(i); // These notes are off the measure. We'll handle them somewhere else.
				}
				if (TappingMode)
					return;
				else
					break;
			}
		}

		if (Measure+1 < NotesInMeasure.size())
		for (std::vector<GameObject>::iterator i = NotesInMeasure[Measure+1].begin(); 
			i != NotesInMeasure[Measure+1].end(); 
			i++)
		{
			// See if it's a hit.
			if ((Val = i->Hit(SongTime, TappingMode ? i->position : mpos, code == GLFW_PRESS ? true : false, IsAutoplaying, key)) != None)
			{
				// Judge accordingly.
				Lifebar.HitJudgement(Val);
				aJudgement.ChangeJudgement(Val);
				StoreEvaluation(Val);

				// If it's a hold, keep running it until it's done.
				if (i->endTime && Val > Miss)
				{
					NotesHeld.push_back(*i);
					i = NotesInMeasure[Measure+1].erase(i); // These notes are off the measure. We'll handle them somewhere else.
				}
				if (TappingMode)
					return;
				else
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
		}else if (key == 'F')
		{
			FailEnabled = !FailEnabled;
		}
		else if (key == 'T')
		{
			TappingMode = !TappingMode;
		}
		else if (key == GLFW_KEY_ESC)
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
		MixerRemoveStream(Music);
		delete Music;
		Music = NULL;
	}
}

void ScreenGameplay::seekTime(float Time)
{
	Music->seek(Time);
	SongTime = Time - GetDeviceLatency();
	SongTimeLatency = Time;
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
		SongDelta = Music->GetPlaybackTime() - SongTimeLatency;
		SongTime += SongDelta;
		SongTimeLatency += SongDelta;
	}

	if ( ScreenTime > ScreenPauseTime || !ShouldChangeScreenAtEnd ) // we're over the pause?
	{
		if (SongTime <= 0)
		{
			startMusic();
		}

		if (Next) // We have a pending screen?
		{
			return RunNested(TimeDelta); // use that instead.
		}

		RunMeasure(SongDelta);

		Barline.Run(SongDelta, MeasureTime, MeasureTimeElapsed);

		if (SongTime > CurrentDiff->Offset)
		{
			MeasureTimeElapsed += SongDelta;

			if (SongDelta == 0)
			{
				if (!Music || Music->IsStopped())
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

	}

	RenderObjects(TimeDelta);

	if (Lifebar.Health <= 0 && ShouldChangeScreenAtEnd && FailEnabled) // You died? Not an infinite screen?
		Running = false; // gg

	return Running;
}

void ScreenGameplay::RenderObjects(float TimeDelta)
{
	glm::vec2 mpos = GraphMan.GetRelativeMPos();
	Cursor.position = mpos;

	Cursor.rotation += 120 * TimeDelta;
	if (Cursor.rotation > 360)
		Cursor.rotation -= 360;

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
			if (NotesInMeasure.size() && NotesInMeasure.at(Measure-1).size() > 0)
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
			if (NotesInMeasure.size() && NotesInMeasure.at(Measure+1).size() > 0)
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
			if (NotesInMeasure.size() && NotesInMeasure.at(Measure).size() > 0)

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
	std::stringstream info;
	if (IsAutoplaying)
		info << "\nAutoplay";
#ifndef NDEBUG
	info << "\nSongTime: " << SongTime << "\nPlaybackTime: ";
	if (Music)
		info << Music->GetPlaybackTime();
	else
		info << "???";
	info << "\nStreamTime: ";
	if(Music)
		info << Music->GetStreamedTime();
	else
		info << "???";

	info << "\nSongDelta: " << SongDelta;
	info << "\nTimeBuffered: ";
	if (Music)
		info << Music->GetStreamedTime() - Music->GetPlaybackTime();
	else
		info << "???";

#endif
	if (TappingMode)
		info << "\nTapping mode";
	if (!FailEnabled)
		info << "\nFailing Disabled";

#ifdef NDEBUG
	if (EditMode)
#endif
		info << "\nMeasure: " << Measure;
	SongInfo.DisplayText(info.str().c_str(), glm::vec2(0,0));
	Cursor.Render();
}