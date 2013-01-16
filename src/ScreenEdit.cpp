#include "Global.h"
#include "ScreenEdit.h"
#include "GraphicsManager.h"

ScreenEdit::ScreenEdit(IScreen *Parent)
	: ScreenGameplay(Parent)
{
	ShouldChangeScreenAtEnd = false; // So it doesn't go into screen evaluation.
}

void ScreenEdit::Init(Song &Other)
{
	ScreenGameplay::Init (Other);

	// CEGUI stuff ahead...
}

void ScreenEdit::StartPlaying( int _Measure )
{
	ScreenGameplay::Init(MySong);
	Measure = _Measure;
	seekTime( spb(MySong.BPM) * Measure * 4 + MySong.Offset );
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
				// StartPlaying(
			} 
			else if (EditScreenState == Playing) // if we're playing, go back to editing
				EditScreenState = Editing;
		}
	}
}

bool ScreenEdit::Run(float delta)
{
	GraphMan.isGuiInputEnabled = (EditScreenState != Playing);

	// we're playing the song? run the game
	if (EditScreenState == Playing)
		ScreenGameplay::Run(delta);
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