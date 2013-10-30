#include "Global.h"
#include "ScreenEdit.h"
#include "GameWindow.h"
#include "ImageLoader.h"
#include "Audio.h"
#include "FileManager.h"

SoundSample *SavedSound = NULL;

ScreenEdit::ScreenEdit(IScreen *Parent)
	: ScreenGameplay(Parent)
{
	ShouldChangeScreenAtEnd = false; // So it doesn't go into screen evaluation.
	CurrentFraction = 0;
	Measure = 0;
	EditScreenState = Editing;

	GhostObject.SetImage(ImageLoader::LoadSkin("hitcircle.png"));
	GhostObject.Alpha = 0.7f;
	GhostObject.Centered = true;
	GhostObject.SetSize(CircleSize);

	EditInfo.LoadSkinFontImage("font.tga", glm::vec2(18, 32), glm::vec2(34,34), glm::vec2(10,16), 32);
	EditMode = true;
	HeldObject = NULL;
	Mode = Select;
	GridEnabled = false;
	GridCellSize = 16;
}

void ScreenEdit::Cleanup()
{
	ScreenGameplay::Cleanup();
}

void ScreenEdit::Init(Song *Other)
{
	if (Other != NULL)
	{
		if (Other->Difficulties.size() == 0) // No difficulties? Create a new one.
			Other->Difficulties.push_back(new SongInternal::Difficulty());

		ScreenGameplay::Init (Other, 0);
		NotesInMeasure.clear(); 

		if (!Other->Difficulties[0]->Timing.size())
			Other->Difficulties[0]->Timing.push_back(SongInternal::Difficulty::TimingSegment());
	}

	if (!SavedSound)
	{
		SavedSound = new SoundSample((FileManager::GetSkinPrefix() + "save.ogg").c_str());
		MixerAddSample(SavedSound);
	}

	OffsetPrompt.SetPrompt("Please insert offset.");
	BPMPrompt.SetPrompt("Please insert BPM.");
	OffsetPrompt.SetFont(&EditInfo);
	BPMPrompt.SetFont(&EditInfo);

	OffsetPrompt.SetOpen(false);
	BPMPrompt.SetOpen(false);
}

void ScreenEdit::StartPlaying( int32 _Measure )
{
	ScreenGameplay::Init(MySong, 0);
	Measure = _Measure;
	seekTime( TimeAtBeat(*CurrentDiff, Measure * MySong->MeasureLength) );
	savedMeasure = Measure;
}

void ScreenEdit::SwitchPreviewMode()
{
	MySong->Process(false);
	if (EditScreenState == Editing) // if we're editing, start playing the song
	{
		EditScreenState = Playing;
		StartPlaying( Measure );
	} 
	else if (EditScreenState == Playing) // if we're playing, go back to editing
	{
		EditScreenState = Editing;
		Measure = savedMeasure;
		stopMusic();
		RemoveTrash();
	}
}

void ScreenEdit::DecreaseCurrentFraction()
{
	if (HeldObject)
	{
		HeldObject->hold_duration -= 4.0 / (float)CurrentDiff->Measures[Measure].Fraction;
		if (HeldObject->hold_duration < 0)
			HeldObject->hold_duration = 0;
		MySong->Process(false);
	}

	if (CurrentFraction || Measure > 0)
	{
		CurrentFraction--;

		if (CurrentFraction > CurrentDiff->Measures[Measure].Fraction) // overflow
		{
			if(CurrentDiff->Measures[Measure-1].Fraction)
			{
				CurrentFraction = CurrentDiff->Measures[Measure-1].Fraction-1;
			}else
				CurrentFraction = 0;

			if (Measure > 0) // Go back a measure
				Measure--;
		}
	}
}

void ScreenEdit::IncreaseCurrentFraction()
{
	if (HeldObject)
	{
		HeldObject->hold_duration += 4.0 / (float)CurrentDiff->Measures[Measure].Fraction;
		MySong->Process(false);
	}

	CurrentFraction++;

	if (CurrentFraction >= CurrentDiff->Measures[Measure].Fraction)
	{
		CurrentFraction = 0;
		if (Measure+1 < CurrentDiff->Measures.size()) // Advance a measure
			Measure++;
	}
}

void ScreenEdit::SaveChart()
{
	String DefaultPath = MySong->ChartFilename.length() ? MySong->ChartFilename : "chart.dcf";
	MySong->Repack();
	MySong->Save((MySong->SongDirectory + String("/") + DefaultPath).c_str());
	SavedSound->Reset();
	MySong->Process();
}

void ScreenEdit::InsertMeasure()
{
	CurrentDiff->Measures.resize(CurrentDiff->Measures.size()+1);
	Measure = CurrentDiff->Measures.size()-1;
	CurrentFraction = 0;
	if (Measure > 0)
		AssignFraction(Measure, CurrentDiff->Measures[Measure-1].Fraction);
	else
		AssignFraction(Measure, 2);
}

void ScreenEdit::HandleInput(int32 key, KeyEventType code, bool isMouseInput)
{
	KeyType tkey = BindingsManager::TranslateKey(key);

	if (EditScreenState == Playing)
	{
		ScreenGameplay::HandleInput(key, code, isMouseInput);
	}

	if (!isMouseInput)
	{
		if (key == 'P') // pressed p?
		{
			SwitchPreviewMode();
		}

		if (EditScreenState != Playing)
		{
			int R = OffsetPrompt.HandleInput(key, code, isMouseInput);

			if (R == 1)
				return;
			else if (R == 2)
			{
				if (Utility::IsNumeric(OffsetPrompt.GetContents().c_str()))
				{
					CurrentDiff->Offset = atof(OffsetPrompt.GetContents().c_str());
				}
				return;
			}

			R = BPMPrompt.HandleInput(key, code, isMouseInput);
			
			if (R == 1)
				return;
			else if (R == 2)
			{
				if (Utility::IsNumeric(BPMPrompt.GetContents().c_str()))
				{
					CurrentDiff->Timing[0].Value = atof(BPMPrompt.GetContents().c_str());
				}
				return;
			}


			if (code == KE_Press)
			{
				switch (tkey)
				{
				case KT_Right:              IncreaseCurrentFraction(); return;
				case KT_Left:               DecreaseCurrentFraction(); return;
				case KT_Escape:             Running = false; return;
				case KT_FractionDec:        if (CurrentDiff->Measures.size()) AssignFraction(Measure, CurrentDiff->Measures[Measure].Fraction-1); return;
				case KT_FractionInc:        if (CurrentDiff->Measures.size()) AssignFraction(Measure, CurrentDiff->Measures[Measure].Fraction+1); return;
				case KT_GridDec:            GridCellSize--; return;
				case KT_GridInc:            GridCellSize++; return;
				case KT_SwitchOffsetPrompt: OffsetPrompt.SwitchOpen(); return;
				case KT_SwitchBPMPrompt:	BPMPrompt.SwitchOpen(); return;
				}
				
				switch (key)
				{
				case 'S': SaveChart(); return;
				case 'Q': 
					if (Mode == Select)
						Mode = Normal;
					else if (Mode == Normal)
						Mode = Hold;
					else
						Mode = Select;
					return;
				case 'R':
					if (Measure > 0)
						AssignFraction(Measure, CurrentDiff->Measures.at(Measure-1).Fraction);
					return;
				case 'T':
					InsertMeasure();
					return;
				case 'X':
					if (Measure+1 < CurrentDiff->Measures.size())
						Measure++;
					return;
				case 'Z':
					if (Measure > 0)
						Measure--;
					return;
				case 'G': GridEnabled = !GridEnabled; return;
				}
			}
		}
	}else // mouse input
	{
		if (EditScreenState == Editing)
		{
			if (Mode != Select && code == KE_Press)
			{
				if (Measure >= 0 && Measure < CurrentDiff->Measures.size())
				{
					if (CurrentDiff->Measures.at(Measure).MeasureNotes.size() > CurrentFraction)
					{
						if (tkey == KT_Select)
						{
							if (Mode == Normal)
							{
								CurrentDiff->Measures.at(Measure).MeasureNotes.at(CurrentFraction).SetPositionX(GhostObject.GetPosition().x);
								CurrentDiff->Measures.at(Measure).MeasureNotes.at(CurrentFraction).MeasurePos = CurrentFraction;
							}else if (Mode == Hold)
							{
								HeldObject = &CurrentDiff->Measures.at(Measure).MeasureNotes.at(CurrentFraction);
								HeldObject->SetPositionX(GhostObject.GetPosition().x);
								HeldObject->MeasurePos = CurrentFraction;
								HeldObject->endTime = 0;
								HeldObject->hold_duration = 0;
							}
						}else
						{
							HeldObject = NULL;
							CurrentDiff->Measures.at(Measure).MeasureNotes.at(CurrentFraction).SetPositionX(0);
							CurrentDiff->Measures.at(Measure).MeasureNotes.at(CurrentFraction).endTime = 0;
							CurrentDiff->Measures.at(Measure).MeasureNotes.at(CurrentFraction).hold_duration = 0;
						}

						MySong->Process(false);
					}
				}
			}

			if (Mode == Hold && code == KE_Release)
			{
				if (tkey == KT_Select)
				{
					HeldObject = NULL;
				}
			} // Held Note, Mouse Button Release
		}

	}
}

void ScreenEdit::RunGhostObject()
{
	GhostObject.SetPositionY(YLock);

	if (!GridEnabled)
	{
		GhostObject.SetPositionX(WindowFrame.GetRelativeMPos().x);
	}else
	{
		int32 CellSize = ScreenWidth / GridCellSize;
		int32 Mod = (int)(WindowFrame.GetRelativeMPos().x - ScreenDifference) % CellSize;
		GhostObject.SetPositionX((int)WindowFrame.GetRelativeMPos().x - Mod);
	}

	if ((GhostObject.GetPosition().x-ScreenDifference) > PlayfieldWidth)
		GhostObject.SetPositionX(PlayfieldWidth+ScreenDifference);
	if ((GhostObject.GetPosition().x-ScreenDifference) < 0)
		GhostObject.SetPositionX(ScreenDifference);

	if (Mode != Select)
		GhostObject.Render();
}

void ScreenEdit::DrawInformation()
{
	std::stringstream info;
	info << "Beat: " << (float)Measure * MySong->MeasureLength + ((float)CurrentFraction) / (float)MySong->MeasureLength;
	if (CurrentDiff->Measures.size())
		info << "\nMaxFrac: " << CurrentDiff->Measures.at(Measure).Fraction;
	info << "\nMode:    ";
	if (Mode == Normal)
		info << "Normal";
	else if (Mode == Hold)
		info << "Hold";
	else
		info << "Null";
	if (GridEnabled)
		info << "\nGrid Enabled (size " << ScreenWidth / GridCellSize << ")";
	EditInfo.DisplayText(info.str().c_str(), glm::vec2(512, 600));
}

void ScreenEdit::CalculateVerticalLock()
{
	if (CurrentDiff->Measures.size())
	{
		if (! (Measure % 2) )
			YLock =  ((float)CurrentFraction / (float)CurrentDiff->Measures[Measure].Fraction) * (float)PlayfieldHeight;
		else
			YLock =  PlayfieldHeight - ((float)CurrentFraction / (float)CurrentDiff->Measures[Measure].Fraction) * (float)PlayfieldHeight;

		YLock += ScreenOffset;
	}
}

bool ScreenEdit::Run(double delta)
{
	WindowFrame.isGuiInputEnabled = (EditScreenState != Playing);

	// we're playing the song? run the game
	if (EditScreenState == Playing)
	{
		if (Music->IsStopped())
			startMusic();
		ScreenGameplay::Run(delta);
		RenderObjects(delta);
	}
	else // editing the song? run the editor
	{
		CalculateVerticalLock();
		RenderObjects(delta);

		if (CurrentDiff->Measures.size())
		{
			double Ratio =  (float)CurrentFraction / (float)CurrentDiff->Measures[Measure].Fraction;
			Barline.Run(delta, Ratio);
			if (Measure > 0)
				DrawVector(CurrentDiff->Measures.at(Measure-1).MeasureNotes, delta);
			DrawVector(CurrentDiff->Measures[Measure].MeasureNotes, delta);
		}

		RunGhostObject();
		DrawInformation();
		OffsetPrompt.Render();
		BPMPrompt.Render();
	}

	return Running;
}

void ScreenEdit::AssignFraction(int Measure, int Fraction)
{
	if (Fraction > 0)
	{
		CurrentDiff->Measures.at(Measure).Fraction = Fraction;
		CurrentDiff->Measures.at(Measure).MeasureNotes.resize(CurrentDiff->Measures[Measure].Fraction);


		// Reassign measure positions.
		uint32 count = 0;
		for (std::vector<GameObject>::iterator i = CurrentDiff->Measures.at(Measure).MeasureNotes.begin(); 
			i != CurrentDiff->Measures.at(Measure).MeasureNotes.end(); 
			i++)
		{
			i->MeasurePos = count;
			count++;
		}
	}
}