#include "GameGlobal.h"
#include "GameState.h"
#include "GraphObject2D.h"
#include "Configuration.h"
#include "BitmapFont.h"
#include "ScreenEvaluation7K.h"
#include "GameWindow.h"
#include "ImageLoader.h"
#include "Audio.h"
#include "ScoreKeeper7K.h"
#include "GraphObjectMan.h"
#include "LuaManager.h"

#include <iostream>
#include <iomanip>

AudioStream *ScreenEvaluation7KMusic = NULL;

ScreenEvaluation7K::ScreenEvaluation7K(Screen *Parent) :
	Screen(Parent)
{
	Running = true;
	Font = NULL;
}

void ScreenEvaluation7K::Init(ScoreKeeper7K *Result)
{
	Score = Result;

	Objects = new GraphObjectMan;
	SetupScorekeeper7KLuaInterface(Objects->GetEnv()->GetState());
	SetScorekeeper7KInstance(Objects->GetEnv()->GetState(), Result);
	GameState::GetInstance().InitializeLua(Objects->GetEnv()->GetState());

	Objects->Initialize(GameState::GetInstance().GetSkinPrefix() + "screenevaluation7k.lua");

	std::cerr << Result->getHistogram();

	Background.SetImage(GameState::GetInstance().GetSkinImage(Configuration::GetSkinConfigs("EvaluationBackground7K")));
	Background.AffectedByLightning = true;
	Background.Centered = 1;
	Background.SetPosition( ScreenWidth / 2, ScreenHeight / 2 );
}


void ScreenEvaluation7K::HandleInput(int32 key, KeyEventType code, bool isMouseInput)
{
	if ((BindingsManager::TranslateKey(key) == KT_Escape || BindingsManager::TranslateKey(key) == KT_Select) && code == KE_Press)
		Running = false;
}

void ScreenEvaluation7K::Cleanup()
{
	delete Objects;
}

bool ScreenEvaluation7K::Run(double Delta)
{
	WindowFrame.SetLightMultiplier(sin(GetScreenTime()) * 0.2 + 1);

	Background.Render();

	Objects->DrawTargets(Delta);
	return Running;
}
