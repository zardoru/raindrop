#include "GameGlobal.h"
#include "GameState.h"
#include "GraphObject2D.h"
#include "ActorJudgement.h"
#include "ImageLoader.h"

#define AnimDuration 0.3f

ActorJudgement::ActorJudgement()
{
	Centered = Configuration::GetSkinConfigf("Centered", "Judgement" ) != 0;
	SetRotation( Configuration::GetSkinConfigf("Rotation", "Judgement" ) );
	Alpha = 0;
	SetImage( GameState::GetInstance().GetSkinImage("judge-perfect.png") );
	SetPosition(Configuration::GetSkinConfigf("X", "Judgement" ), Configuration::GetSkinConfigf("Y", "Judgement" ) );
	AnimTime = 0;
	AffectedByLightning = true;
}

void ActorJudgement::ChangeJudgement(Judgement New)
{
	AnimTime = AnimDuration;
	SetScale(1.3f);
	Alpha = 1;

	switch (New)
	{
	case Excellent:
		SetImage(GameState::GetInstance().GetSkinImage("judge-excellent.png"));
		break;
	case Perfect:
		SetImage(GameState::GetInstance().GetSkinImage("judge-perfect.png"));
		break;
	case Great:
		SetImage(GameState::GetInstance().GetSkinImage("judge-great.png"));
		break;
	case Bad:
		SetImage(GameState::GetInstance().GetSkinImage("judge-bad.png"));
		break;
	case Miss:
		SetImage(GameState::GetInstance().GetSkinImage("judge-miss.png"));
		break;
	case None:
		break;
	}
}

void ActorJudgement::Run(double delta)
{
	if (AnimTime > 0)
	{
		SetScale( Vec2(LerpRatio (GetScale().x, 1.0f, AnimDuration - AnimTime, AnimDuration),
			LerpRatio (GetScale().y, 1.0f, AnimDuration - AnimTime, AnimDuration) ) );
	}

	AnimTime -= delta;

	if (AnimTime < -1)
	{
		Alpha -= delta; // Fade out in one second
	}
}