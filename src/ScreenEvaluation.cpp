#include "Global.h"
#include "Game_Consts.h"
#include "ScreenEvaluation.h"
#include "GraphicsManager.h"
#include "ImageLoader.h"
#include "Audio.h"


ScreenEvaluation::ScreenEvaluation(IScreen *Parent) :
	IScreen(Parent)
{
	Running = true;
}

int ScreenEvaluation::CalculateScore()
{
	return Results.NumExcellents * 10000 +
		Results.NumPerfects * 8500 +
		Results.NumGreats * 4500 +
		Results.NumOK * 2000 +
		Results.NumBads * 1000;
		
}

void ScreenEvaluation::Init(EvaluationData _Data)
{
#ifndef DISABLE_CEGUI
	Results = _Data;

	// GUI stuff.
	using namespace CEGUI;
	char _Results[256];
	const char *Text = "Excellent: \n"
					  "Perfect:   \n"
					  "Great:     \n"
					  "Bad:       \n"
					  "NG:        \n"
					  "OK:        \n"
					  "Misses:    \n"
					  "\n"
					  "Max Combo: \n"
					  "Score:     \n";
	sprintf(_Results, "%d\n%d\n%d\n%d\n%d\n%d\n%d\n\n%d\n%d\n",
	Results.NumExcellents,
		Results.NumPerfects,
		Results.NumGreats,
		Results.NumBads,
		Results.NumNG,
		Results.NumOK,
		Results.NumMisses,
		Results.MaxCombo,
		CalculateScore());

	GraphMan.isGuiInputEnabled = true;


	WindowManager& winMgr = WindowManager::getSingleton();
	root = (DefaultWindow*)winMgr.createWindow("DefaultWindow", "ScreenEvalRoot");

	FrameWindow* fWnd = static_cast<FrameWindow*>(
		winMgr.createWindow( "TaharezLook/FrameWindow", "evalWindow" ));

	System::getSingleton().setGUISheet(root);

	fWnd->setText("Evaluation Results!");
	fWnd->setPosition(UVector2(cegui_reldim(0.25f), cegui_reldim( 0.25f)));
    fWnd->setSize(UVector2(cegui_reldim(0.5f), cegui_reldim( 0.5f)));
	root->addChildWindow(fWnd);

	Window* st = winMgr.createWindow("TaharezLook/StaticText", "EvaluationText");
    fWnd->addChildWindow(st);
	st->setText(Text);
	st->setPosition(UVector2(cegui_reldim(0), cegui_reldim(0)));
	st->setSize(UVector2(cegui_reldim(0.5f), cegui_reldim(0.85)));

	Window* st2 = winMgr.createWindow("TaharezLook/StaticText", "EvaluationResults");
	st2->setText(_Results);
	st2->setPosition(UVector2(cegui_reldim(0.5f), cegui_reldim( 0)));
	st2->setSize(UVector2(cegui_reldim(0.5f), cegui_reldim(0.85)));
	fWnd->addChildWindow(st2);


		PushButton *btn = static_cast<PushButton*>(winMgr.createWindow("TaharezLook/Button", "FinishEvalScreenButton"));
    fWnd->addChildWindow(btn);
    btn->setPosition(UVector2(cegui_reldim(0.25f), cegui_reldim( 0.93f)));
    btn->setSize(UVector2(cegui_reldim(0.50f), cegui_reldim( 0.05f)));
    btn->setText("Return To Music Selection");

	// Button that activates the music. Oh yeah!
	winMgr.getWindow("FinishEvalScreenButton")->
		subscribeEvent(PushButton::EventClicked, Event::Subscriber(&ScreenEvaluation::StopRunning, this));
#endif
}

bool ScreenEvaluation::StopRunning(const CEGUI::EventArgs&)
{
	Running = false;
	return true;
}

void ScreenEvaluation::HandleInput(int key, int code, bool isMouseInput)
{
	if (key == GLFW_KEY_ESC && code == GLFW_PRESS)
		Running = false;
}

void ScreenEvaluation::Cleanup()
{
#ifndef DISABLE_CEGUI
	using namespace CEGUI;
	WindowManager& winMgr = WindowManager::getSingleton();
	winMgr.destroyWindow("ScreenEvalRoot"); // Remove our silly root window.
#endif
}

bool ScreenEvaluation::Run(float)
{
#ifndef DISABLE_CEGUI
	CEGUI::System::getSingleton().renderGUI();
#endif
	return Running;
}