#include <stdio.h>
#include "GameGlobal.h"
#include "GameState.h"
#include "GraphObject2D.h"
#include "BitmapFont.h"
#include "ScreenEvaluation.h"
#include "GameWindow.h"
#include "ImageLoader.h"
#include "Audio.h"

AudioStream *ScreenEvaluationMusic = NULL;

ScreenEvaluation::ScreenEvaluation(Screen *Parent) :
	Screen(Parent)
{
	Running = true;
	Font = NULL;
}

int32 ScreenEvaluation::CalculateScore()
{
	return int32(1000000.0 * Results.dpScoreSquare / (double)(Results.totalNotes * (Results.totalNotes + 1)));
}

void ScreenEvaluation::Init(EvaluationData _Data, GString SongAuthor, GString SongTitle)
{
	if (!ScreenEvaluationMusic)
	{
		ScreenEvaluationMusic = new SoundStream();
		ScreenEvaluationMusic->Open((GameState::GetInstance().GetSkinPrefix() + "screenevaluationloop.ogg").c_str());
		ScreenEvaluationMusic->SetLoop(true);
		MixerAddStream(ScreenEvaluationMusic);
	}

	ScreenEvaluationMusic->SeekTime(0);
	ScreenEvaluationMusic->Play();


	Background.SetImage(GameState::GetInstance().GetSkinImage(Configuration::GetSkinConfigs("EvaluationBackground")));
	Background.AffectedByLightning = true;
	if (!Font)
	{
		Font = new BitmapFont();
		Font->LoadSkinFontImage("font_screenevaluation.tga", Vec2(10, 20), Vec2(32, 32), Vec2(10,20), 32);
		Font->SetAffectedByLightning(true);
	}
	Results = _Data;

	char _Results[256];
	const char *Text ="Excellent: \n"
					  "Perfect:   \n"
					  "Great:     \n"
					  "Bad:       \n"
					  "NG:        \n"
					  "OK:        \n"
					  "Misses:    \n"
					  "\n"
					  "Max Combo: \n"
					  "Score:     \n";

	ResultsGString = Text;

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

	ResultsNumerical = _Results;
	WindowFrame.SetLightMultiplier(1);
	WindowFrame.SetLightPosition(glm::vec3(0,0,1));

	TitleFormat = SongTitle + " by " + SongAuthor;
}


void ScreenEvaluation::HandleInput(int32 key, KeyEventType code, bool isMouseInput)
{
	if ((BindingsManager::TranslateKey(key) == KT_Escape || BindingsManager::TranslateKey(key) == KT_Select) && code == KE_Press)
		Running = false;
}

void ScreenEvaluation::Cleanup()
{
	ScreenEvaluationMusic->Stop();
	delete Font;
}

bool ScreenEvaluation::Run(double Delta)
{
	WindowFrame.SetLightMultiplier(sin(GetScreenTime()) * 0.2 + 1);

	Background.Render();
	if (Font)
	{
		Font->Render(ResultsGString,        Vec2( ScreenWidth/2 - 110, ScreenHeight/2 - 100 ));
		Font->Render(ResultsNumerical,     Vec2( ScreenWidth/2, ScreenHeight/2 - 100 ));
		Font->Render(GString("results screen"),			    Vec2( ScreenWidth/2 - 70, 0 ));
		Font->Render(GString("press space to continue..."), Vec2( ScreenWidth/2 - 130, ScreenHeight*7/8 ));
		Font->Render(TitleFormat, Vec2( 0, ScreenHeight - 20 ));
	}
	return Running;
}
