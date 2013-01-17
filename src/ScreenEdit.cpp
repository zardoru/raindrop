#include "Global.h"
#include "ScreenEdit.h"
#include "GraphicsManager.h"
#include <boost/lexical_cast.hpp>

ScreenEdit::ScreenEdit(IScreen *Parent)
	: ScreenGameplay(Parent)
{
	ShouldChangeScreenAtEnd = false; // So it doesn't go into screen evaluation.
	GuiInitialized = false;
	measureFrac = 0;
}

void ScreenEdit::Init(Song *Other)
{
	if (Other != NULL)
		ScreenGameplay::Init (Other);

	if (GuiInitialized)
		return;

	// CEGUI stuff ahead...
	using namespace CEGUI;

	WindowManager& winMgr = WindowManager::getSingleton();
	root = (DefaultWindow*)winMgr.createWindow("DefaultWindow", "ScreenEditRoot");

	System::getSingleton().setGUISheet(root);
	
	FrameWindow* fWnd = static_cast<FrameWindow*>(winMgr.createWindow( "TaharezLook/FrameWindow", "screenWindow" ));
	fWnd->setPosition(UVector2 (cegui_reldim(0), cegui_reldim(0)));
	fWnd->setSize(UVector2 (cegui_reldim(0.25), cegui_reldim(1)));
	root->addChildWindow(fWnd);

	Window* st = winMgr.createWindow("TaharezLook/StaticText", "BPMText");
    fWnd->addChildWindow(st);
	st->setText("BPM:");
	st->setPosition(UVector2(cegui_reldim(0.0f), cegui_reldim(0.1)));
	st->setSize(UVector2(cegui_reldim(0.3f), cegui_reldim(0.07f)));

	BPMBox = static_cast<Editbox*> (winMgr.createWindow("TaharezLook/Editbox", "BPMBox"));
	BPMBox->setSize(UVector2 (cegui_reldim(0.8), cegui_reldim(0.15)));
	BPMBox->setPosition(UVector2 (cegui_reldim(0.1), cegui_reldim(0.2)));
	fWnd->addChildWindow(BPMBox);

	Window* st2 = winMgr.createWindow("TaharezLook/StaticText", "MsrText");
    fWnd->addChildWindow(st2);
	st2->setText("Measure:");
	st2->setPosition(UVector2(cegui_reldim(0.23f), cegui_reldim(0.1)));
	st2->setSize(UVector2(cegui_reldim(0.3f), cegui_reldim(0.07f)));

	CurrentMeasure = static_cast<Editbox*> (winMgr.createWindow("TaharezLook/Editbox", "MeasureBox"));
	CurrentMeasure->setSize(UVector2 (cegui_reldim(0.8), cegui_reldim(0.15)));
	CurrentMeasure->setPosition(UVector2 (cegui_reldim(0.1), cegui_reldim(0.4)));
	fWnd->addChildWindow(CurrentMeasure);

	winMgr.getWindow("MeasureBox")->
		subscribeEvent(Editbox::EventTextChanged, Event::Subscriber(&ScreenEdit::measureTextChanged, this));

	winMgr.getWindow("BPMBox")->
		subscribeEvent(Editbox::EventTextChanged, Event::Subscriber(&ScreenEdit::bpmTextChanged, this));
	// Editbox::onTextChanged

	GuiInitialized = true;
}

void ScreenEdit::StartPlaying( int _Measure )
{
	ScreenGameplay::Init(MySong);
	Measure = _Measure;
	seekTime( spb(MySong->BPM) * Measure * 4 + MySong->Offset );
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
				stopMusic();
			}
		}
	}
}

bool ScreenEdit::Run(float delta)
{
	GraphMan.isGuiInputEnabled = (EditScreenState != Playing);

	// we're playing the song? run the game
	if (EditScreenState == Playing)
	{
		ScreenGameplay::Run(delta);
	}
	else // editing the song? run the editor
	{
		RenderObjects(delta);
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
		MySong->BPM = boost::lexical_cast<int>(BPMBox->getText());
	}catch (...)
	{
		// hmm.. What to do?
	}

	return true;
}