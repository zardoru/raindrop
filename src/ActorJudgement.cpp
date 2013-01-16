#include "Global.h"
#include "Game_Consts.h"
#include "GraphObject2D.h"
#include "ActorJudgement.h"
#include "ImageLoader.h"
#include "GraphicsManager.h"

#define AnimDuration 0.3

ActorJudgement::ActorJudgement()
{
	origin = 1; // use center
	position.x = GraphMan.GetMatrixSize().x / 2;
	position.y = PlayfieldHeight * 3 / 4 + ScreenOffset;
	alpha = 0;
	red = green = blue = 1;
	setImage(ImageLoader::LoadSkin("judge-perfect.png"));
	GraphObject2D::Init(true);
}

void ActorJudgement::ChangeJudgement(Judgement New)
{
	AnimTime = AnimDuration;
	scaleX = scaleY = 1.3;
	alpha = 1;

	switch (New)
	{
	case Excellent:
		setImage(ImageLoader::LoadSkin("judge-excellent.png"));
		break;
	case Perfect:
		setImage(ImageLoader::LoadSkin("judge-perfect.png"));
		break;
	case Great:
		setImage(ImageLoader::LoadSkin("judge-great.png"));
		break;
	case Bad:
		setImage(ImageLoader::LoadSkin("judge-bad.png"));
		break;
	case Miss:
		setImage(ImageLoader::LoadSkin("judge-miss.png"));
		break;
	case None:
		break;
	}
}

void ActorJudgement::Run(float delta)
{
	if (AnimTime > 0)
	{
		float speed = 1.3 / 0.7;
		scaleX -= speed * delta;
		scaleY -= speed * delta;
		if (scaleX < 1)
			scaleX = 1;
		if (scaleY < 1)
			scaleY = 1;
	}else
	{
		scaleX = scaleY = 0.8;
	}

	AnimTime -= delta;

	if (AnimTime < -1)
	{
		alpha -= delta; // Fade out in one second
	}
}