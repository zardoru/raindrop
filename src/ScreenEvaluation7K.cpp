#include "GameGlobal.h"
#include "GraphObject2D.h"
#include "Configuration.h"
#include "BitmapFont.h"
#include "ScreenEvaluation7K.h"
#include "GameWindow.h"
#include "ImageLoader.h"
#include "Audio.h"
#include "FileManager.h"
#include "ScoreKeeper.h"

AudioStream *ScreenEvaluation7KMusic = NULL;

ScreenEvaluation7K::ScreenEvaluation7K(IScreen *Parent) :
	IScreen(Parent)
{
	Running = true;
	Font = NULL;
}

void ScreenEvaluation7K::Init(ScoreKeeper7K *Result)
{
	Score = Result;

	std::stringstream ss;

	ss << "ex%: " << Result->getPercentScore(PST_EX) << "\n"
		<< "notes hit%: " << Result->getPercentScore(PST_NH) << "\n"
		<< "acc%: " << Result->getPercentScore(PST_ACC) << "\n"
		<< "max combo: " << Result->getScore(ST_MAX_COMBO);
	
	DisplayResult = ss.str();

	Background.SetImage(ImageLoader::LoadSkin(Configuration::GetSkinConfigs("EvaluationBackground7K")));
	Background.AffectedByLightning = true;
	Background.Centered = 1;
	Background.SetPosition( ScreenWidth / 2, ScreenHeight / 2 );

	if (!Font)
	{
		Font = new BitmapFont();
		Font->LoadSkinFontImage("font_screenevaluation.tga", Vec2(10, 20), Vec2(32, 32), Vec2(10,20), 32);
		Font->SetAffectedByLightning(true);
	}
}


void ScreenEvaluation7K::HandleInput(int32 key, KeyEventType code, bool isMouseInput)
{
	if ((BindingsManager::TranslateKey(key) == KT_Escape || BindingsManager::TranslateKey(key) == KT_Select) && code == KE_Press)
		Running = false;
}

void ScreenEvaluation7K::Cleanup()
{
	delete Font;
}

bool ScreenEvaluation7K::Run(double Delta)
{
	ScreenTime += Delta;

	WindowFrame.SetLightMultiplier(sin(ScreenTime) * 0.2 + 1);

	Background.Render();
	if (Font)
	{
		Font->DisplayText("results screen",			    Vec2( ScreenWidth/2 - 70, 0 ));
		Font->DisplayText("press space to continue...", Vec2( ScreenWidth/2 - 130, ScreenHeight*7/8 ));

		Font->DisplayText(DisplayResult.c_str(), Vec2( ScreenWidth / 2 - 200, ScreenHeight / 2 - 200 ));
	}
	return Running;
}