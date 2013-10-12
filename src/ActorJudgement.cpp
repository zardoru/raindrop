#include "Global.h"
#include "Game_Consts.h"
#include "GraphObject2D.h"
#include "ActorJudgement.h"
#include "ImageLoader.h"
#include "GraphicsManager.h"

#define AnimDuration 0.3f

ActorJudgement::ActorJudgement()
{
	Centered = 1;
	SetPosition(GraphMan.GetMatrixSize().x / 2, PlayfieldHeight * 3 / 4 + ScreenOffset);
	Alpha = 0;
	SetImage(ImageLoader::LoadSkin("judge-perfect.png"));
}

void ActorJudgement::ChangeJudgement(Judgement New)
{
	AnimTime = AnimDuration;
	SetScale(1.3);
	Alpha = 1;

	switch (New)
	{
	case Excellent:
		SetImage(ImageLoader::LoadSkin("judge-excellent.png"));
		break;
	case Perfect:
		SetImage(ImageLoader::LoadSkin("judge-perfect.png"));
		break;
	case Great:
		SetImage(ImageLoader::LoadSkin("judge-great.png"));
		break;
	case Bad:
		SetImage(ImageLoader::LoadSkin("judge-bad.png"));
		break;
	case Miss:
		SetImage(ImageLoader::LoadSkin("judge-miss.png"));
		break;
	case None:
		break;
	}
}

void ActorJudgement::Run(double delta)
{
	if (AnimTime > 0)
	{
		float speed = 1.3f / 0.7f;
		SetScale(glm::vec2( GetScale().x - speed * delta, GetScale().y - speed * delta));
		if (GetScale().x < 1)
			SetScaleX(1);
		if (GetScale().y < 1)
			SetScaleY(1);
	}else
	{
		SetScale(0.8f);
	}

	AnimTime -= delta;

	if (AnimTime < -1)
	{
		Alpha -= delta; // Fade out in one second
	}
}