#include "Global.h"
#include "ScreenEdit.h"
#include "GraphicsManager.h"
#include "ImageLoader.h"
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

ScreenEdit::ScreenEdit(IScreen *Parent)
	: ScreenGameplay(Parent)
{
	ShouldChangeScreenAtEnd = false; // So it doesn't go into screen evaluation.
	GuiInitialized = false;
	CurrentFraction = 0;
	EditScreenState = Editing;
	GhostObject.setImage(ImageLoader::LoadSkin("hitcircle.png"));
	GhostObject.alpha = 0.7f;
	GhostObject.origin = 1;
	GhostObject.width = GhostObject.height = CircleSize;
}

void ScreenEdit::Init(Song *Other)
{
	if (Other != NULL)
	{
		Other->Difficulties.push_back(new SongInternal::Difficulty());
		ScreenGameplay::Init (Other);
		Other->Difficulties[0]->Timing.push_back(SongInternal::Difficulty::TimingSegment());
	}

	if (GuiInitialized)
		return;

	// CEGUI stuff ahead...
	using namespace CEGUI;

	WindowManager& winMgr = WindowManager::getSingleton();
	root = (DefaultWindow*)winMgr.createWindow("DefaultWindow", "ScreenEditRoot");

	System::getSingleton().setGUISheet(root);
	
	FrameWindow* fWnd = static_cast<FrameWindow*>(winMgr.createWindow( "TaharezLook/FrameWindow", "screenEditWindow" ));
	fWnd->setPosition(UVector2 (cegui_reldim(0.f), cegui_reldim(0.f)));
	fWnd->setSize(UVector2 (cegui_reldim(0.25f), cegui_reldim(0.85f)));
	root->addChildWindow(fWnd);

	Window* st = winMgr.createWindow("TaharezLook/StaticText", "BPMText");
    fWnd->addChildWindow(st);
	st->setText("BPM");
	st->setPosition(UVector2(cegui_reldim(0.1f), cegui_reldim(0.04f)));
	st->setSize(UVector2(cegui_reldim(0.3f), cegui_reldim(0.04f)));

	BPMBox = static_cast<Editbox*> (winMgr.createWindow("TaharezLook/Editbox", "BPMBox"));
	BPMBox->setSize(UVector2 (cegui_reldim(0.8f), cegui_reldim(0.15f)));
	BPMBox->setPosition(UVector2 (cegui_reldim(0.1f), cegui_reldim(0.07f)));
	fWnd->addChildWindow(BPMBox);

	Window* st2 = winMgr.createWindow("TaharezLook/StaticText", "MsrText");
    fWnd->addChildWindow(st2);
	st2->setText("Measure");
	st2->setPosition(UVector2(cegui_reldim(0.1f), cegui_reldim(0.5f)));
	st2->setSize(UVector2(cegui_reldim(0.5f), cegui_reldim(0.04f)));

	Window* st3 = winMgr.createWindow("TaharezLook/StaticText", "FraText");
    fWnd->addChildWindow(st3);
	st3->setText("Fraction");
	st3->setPosition(UVector2(cegui_reldim(0.1f), cegui_reldim(0.3f)));
	st3->setSize(UVector2(cegui_reldim(0.5f), cegui_reldim(0.04f)));

	Window* st4 = winMgr.createWindow("TaharezLook/StaticText", "OffsText");
    fWnd->addChildWindow(st4);
	st4->setText("Offset");
	st4->setPosition(UVector2(cegui_reldim(0.1f), cegui_reldim(0.7f)));
	st4->setSize(UVector2(cegui_reldim(0.3f), cegui_reldim(0.04f)));

	/*
	st2->setVerticalAlignment(VA_TOP);
	st2->setHorizontalAlignment(HA_CENTRE);
	*/

	CurrentMeasure = static_cast<Editbox*> (winMgr.createWindow("TaharezLook/Editbox", "MeasureBox"));
	CurrentMeasure->setSize(UVector2 (cegui_reldim(0.8f), cegui_reldim(0.1f)));
	CurrentMeasure->setPosition(UVector2 (cegui_reldim(0.1f), cegui_reldim(0.4f)));

	fWnd->addChildWindow(CurrentMeasure);

	OffsetBox = static_cast<Editbox*> (winMgr.createWindow("TaharezLook/Editbox", "OffsetBox"));
	OffsetBox->setSize(UVector2 (cegui_reldim(0.8f), cegui_reldim(0.1f)));
	OffsetBox->setPosition(UVector2 (cegui_reldim(0.1f), cegui_reldim(0.6f)));
	fWnd->addChildWindow(OffsetBox);

	FracBox = static_cast<Editbox*> (winMgr.createWindow("TaharezLook/Editbox", "FracBox"));
	FracBox->setSize(UVector2 (cegui_reldim(0.8f), cegui_reldim(0.05f)));
	FracBox->setPosition(UVector2 (cegui_reldim(0.1f), cegui_reldim(0.25f)));
	fWnd->addChildWindow(FracBox);

	winMgr.getWindow("MeasureBox")->
		subscribeEvent(Editbox::EventTextChanged, Event::Subscriber(&ScreenEdit::measureTextChanged, this));

	winMgr.getWindow("BPMBox")->
		subscribeEvent(Editbox::EventTextChanged, Event::Subscriber(&ScreenEdit::bpmTextChanged, this));

	winMgr.getWindow("OffsetBox")->
		subscribeEvent(Editbox::EventTextChanged, Event::Subscriber(&ScreenEdit::offsetTextChanged, this));

	winMgr.getWindow("FracBox")->
		subscribeEvent(Editbox::EventTextChanged, Event::Subscriber(&ScreenEdit::fracTextChanged, this));

	// information window
	fWndInfo = static_cast<FrameWindow*>(winMgr.createWindow( "TaharezLook/FrameWindow", "screenInfoWindow" ));
	fWndInfo->setPosition(UVector2 (cegui_reldim(0.75), cegui_reldim(0)));
	fWndInfo->setSize(UVector2 (cegui_reldim(0.25), cegui_reldim(0.25)));
	root->addChildWindow(fWndInfo);

	GuiInitialized = true;
}

void ScreenEdit::StartPlaying( int _Measure )
{
	ScreenGameplay::Init(MySong);
	Measure = _Measure;
	seekTime( spb(CurrentDiff->Timing[0].Value) * Measure * 4 + CurrentDiff->Offset);
	startMusic();
	savedMeasure = Measure;
}

void ScreenEdit::HandleInput(int key, int code, bool isMouseInput)
{
	if (EditScreenState == Playing)
		ScreenGameplay::HandleInput(key, code, isMouseInput);

	if (!isMouseInput)
	{
		if (key == 'P') // pressed p?
		{
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
		if (code == GLFW_PRESS)
		{
			if (key == GLFW_KEY_RIGHT)
			{
				CurrentFraction++;

				if (CurrentFraction >= CurrentDiff->Measures[Measure].Fraction)
				{
					CurrentFraction = 0;
				}
			}else if (key == GLFW_KEY_LEFT)
			{
				CurrentFraction--;

				if (CurrentFraction > CurrentDiff->Measures[Measure].Fraction) // overflow
				{
					CurrentFraction = CurrentDiff->Measures[Measure].Fraction-1;
				}
			}
		}
	}else // mouse input
	{
		if (EditScreenState == Editing)
		{
			try {
				if (key == GLFW_MOUSE_BUTTON_LEFT)
				{
					CurrentDiff->Measures.at(Measure).MeasureNotes.at(CurrentFraction).position.x = GhostObject.position.x;
					CurrentDiff->Measures.at(Measure).MeasureNotes.at(CurrentFraction).MeasurePos = CurrentFraction;
				}else
				{
					CurrentDiff->Measures.at(Measure).MeasureNotes.at(CurrentFraction).position.x = 0;
				}
				MySong->Process(false);
			} catch (...){}
		}

	}
}

bool ScreenEdit::Run(double delta)
{
	GraphMan.isGuiInputEnabled = (EditScreenState != Playing);

	// we're playing the song? run the game
	if (EditScreenState == Playing)
	{
		ScreenGameplay::Run(delta);
		RenderObjects(delta);
	}
	else // editing the song? run the editor
	{
		std::stringstream strs;
		strs << "x: " << GraphMan.GetRelativeMPos().x - 112 << " y: " << GraphMan.GetRelativeMPos().y;
		fWndInfo->setText(strs.str());

		if (CurrentDiff->Measures.size())
		{
			Barline.Run(delta, CurrentDiff->Measures[Measure].Fraction, CurrentFraction);
			if (! (Measure % 2 ))
				YLock =  ((float)CurrentFraction / (float)CurrentDiff->Measures[Measure].Fraction) * (float)PlayfieldHeight;
			else
				YLock =  PlayfieldHeight - ((float)CurrentFraction / (float)CurrentDiff->Measures[Measure].Fraction) * (float)PlayfieldHeight;

			YLock += ScreenOffset;
		}

		GhostObject.position.y = YLock;
		GhostObject.position.x = GraphMan.GetRelativeMPos().x;
		if ((GhostObject.position.x-112) > PlayfieldWidth)
			GhostObject.position.x = PlayfieldWidth+112;
		if ((GhostObject.position.x-112) < 0)
			GhostObject.position.x = 112;

		RenderObjects(delta);

		if (CurrentDiff->Measures.size())
		{
			for (std::vector<GameObject>::iterator i = CurrentDiff->Measures[Measure].MeasureNotes.begin(); i != CurrentDiff->Measures[Measure].MeasureNotes.end(); i++)
			{
				i->Render();
			}
		}

		GhostObject.Render();
		CEGUI::System::getSingleton().renderGUI();
	}

	return Running;
}

void ScreenEdit::Cleanup()
{
	// remove our filthy windows
	CEGUI::WindowManager::getSingleton().destroyWindow("ScreenEditRoot");
}

bool ScreenEdit::measureTextChanged(const CEGUI::EventArgs& param)
{
	try
	{
		Measure = boost::lexical_cast<int>(CurrentMeasure->getText());
		if ((int)Measure > (((int)CurrentDiff->Measures.size())-1))
		{
			CurrentDiff->Measures.resize(Measure+1);
		}else
		{
			FracBox->setText( (boost::format("%d\n") % CurrentDiff->Measures[Measure].Fraction).str().c_str() );
		}
	}catch (...) { /* What to do now? */ }
	return true;
}

bool ScreenEdit::offsetTextChanged(const CEGUI::EventArgs& param)
{
	try 
	{
		CurrentDiff->Offset = boost::lexical_cast<float>(OffsetBox->getText());
	}catch (...)
	{
		// hmm.. What to do?
	}
	return true;
}

bool ScreenEdit::bpmTextChanged(const CEGUI::EventArgs& param)
{
	try 
	{
		CurrentDiff->Timing[0].Value = boost::lexical_cast<int>(BPMBox->getText());
	}catch (...)
	{
		// hmm.. What to do?
	}

	return true;
}

bool ScreenEdit::fracTextChanged(const CEGUI::EventArgs& param)
{
	try
	{
		CurrentDiff->Measures.at(Measure).Fraction = boost::lexical_cast<int>(FracBox->getText());
		CurrentDiff->Measures.at(Measure).MeasureNotes.resize(CurrentDiff->Measures[Measure].Fraction);
	}catch(...)
	{
	}
	return true;
}