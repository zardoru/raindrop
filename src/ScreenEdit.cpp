#include "Global.h"
#include "ScreenEdit.h"
#include "GameWindow.h"
#include "ImageLoader.h"
#include "Audio.h"


ScreenEdit::ScreenEdit(IScreen *Parent)
	: ScreenGameplay(Parent)
{
	ShouldChangeScreenAtEnd = false; // So it doesn't go into screen evaluation.
	GuiInitialized = false;
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

		Other->Difficulties[0]->Timing.push_back(SongInternal::Difficulty::TimingSegment());
	}
}

void ScreenEdit::StartPlaying( int32 _Measure )
{
	ScreenGameplay::Init(MySong, 0);
	Measure = _Measure;
	seekTime( spb(CurrentDiff->Timing[0].Value) * Measure * MySong->MeasureLength + CurrentDiff->Offset);
	savedMeasure = Measure;
}

void ScreenEdit::HandleInput(int32 key, KeyEventType code, bool isMouseInput)
{
	KeyType tkey = BindingsManager::TranslateKey(key);
	if (EditScreenState == Playing)
		ScreenGameplay::HandleInput(key, code, isMouseInput);

	if (!isMouseInput)
	{
		if (key == 'P') // pressed p?
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

		if (EditScreenState != Playing)
		{
			if (code == KE_Press)
			{
				if (tkey == KT_Right)
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

				}else if (tkey == KT_Left)
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
							CurrentFraction = CurrentDiff->Measures[Measure].Fraction-1;

							if (Measure > 0) // Go back a measure
								Measure--;
						}
					}
				}else if (tkey == KT_Escape)
				{
					Running = false; // Get out.
				}else if (tkey == KT_FractionDec)
				{
					AssignFraction(Measure, CurrentDiff->Measures[Measure].Fraction-1);
				}else if (tkey == KT_FractionInc)
				{
					AssignFraction(Measure, CurrentDiff->Measures[Measure].Fraction+1);
				}else if (key == 'S') // Save!
				{
					String DefaultPath = "chart.dcf";
					MySong->Repack();
					MySong->Save((MySong->SongDirectory + String("/") + DefaultPath).c_str());
					MySong->Process();
				}else if (key == 'Q')
				{
					if (Mode == Select)
						Mode = Normal;
					else if (Mode == Normal)
						Mode = Hold;
					else
						Mode = Select;
				}else if (key == 'R') // Repeat previous measure's fraction
				{
					if (Measure > 0)
						AssignFraction(Measure, CurrentDiff->Measures.at(Measure-1).Fraction);
				}else if (key == 'T') // Insert another measure, go to it and restart fraction
				{
					CurrentDiff->Measures.resize(CurrentDiff->Measures.size()+1);
					Measure = CurrentDiff->Measures.size()-1;
					CurrentFraction = 0;
					if (Measure > 0)
						AssignFraction(Measure, CurrentDiff->Measures[Measure-1].Fraction);
					else
						AssignFraction(Measure, 2);
				}else if (key == 'X')
				{
					if (Measure+1 < CurrentDiff->Measures.size())
						Measure++;
				}else if (key == 'Z')
				{
					if (Measure > 0)
						Measure--;
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
		if (CurrentDiff->Measures.size())
		{
			Barline.Run(delta, CurrentDiff->Measures[Measure].Fraction, CurrentFraction);
			if (! (Measure % 2) )
				YLock =  ((float)CurrentFraction / (float)CurrentDiff->Measures[Measure].Fraction) * (float)PlayfieldHeight;
			else
				YLock =  PlayfieldHeight - ((float)CurrentFraction / (float)CurrentDiff->Measures[Measure].Fraction) * (float)PlayfieldHeight;

			YLock += ScreenOffset;
		}

		GhostObject.SetPositionY(YLock);
		GhostObject.SetPositionX(WindowFrame.GetRelativeMPos().x);
		
		if ((GhostObject.GetPosition().x-ScreenDifference) > PlayfieldWidth)
			GhostObject.SetPositionX(PlayfieldWidth+ScreenDifference);
		if ((GhostObject.GetPosition().x-ScreenDifference) < 0)
			GhostObject.SetPositionX(ScreenDifference);

		RenderObjects(delta);

		if (CurrentDiff->Measures.size())
		{
				if (Measure > 0)
				{
					for (std::vector<GameObject>::reverse_iterator i = CurrentDiff->Measures.at(Measure-1).MeasureNotes.rbegin(); i != CurrentDiff->Measures.at(Measure-1).MeasureNotes.rend(); i++)
					{	
						if (i->GetPosition().x > ScreenDifference)
							i->Render();
					}
				}

				for (std::vector<GameObject>::reverse_iterator i = CurrentDiff->Measures[Measure].MeasureNotes.rbegin(); i != CurrentDiff->Measures[Measure].MeasureNotes.rend(); i++)
				{
					if (i->GetPosition().x > ScreenDifference)
						i->Render();
				}
		}

		if (Mode != Select)
			GhostObject.Render();

		std::stringstream info;
		info << "Measure:   " << Measure
			 << "\nFrac:    " << CurrentFraction;
		if (CurrentDiff->Measures.size())
			 info << "\nMaxFrac: " << CurrentDiff->Measures.at(Measure).Fraction;
		info << "\nMode:    ";
		if (Mode == Normal)
			  info << "Normal";
		else if (Mode == Hold)
			info << "Hold";
		else
			info << "Null";
		EditInfo.DisplayText(info.str().c_str(), glm::vec2(512, 600));

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