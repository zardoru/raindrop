#include "Global.h"
#include "Game_Consts.h"
#include "GraphObject2D.h"
#include "ActorLifebar.h"
#include "ImageLoader.h"
#include "GraphicsManager.h"

ActorLifebar::ActorLifebar() : GraphObject2D()
{
	Health = 50; // Out of 100!
	pending_health = 0;
	SetImage(ImageLoader::LoadSkin("healthbar.png"));
	time = 0;
	Centered = false;
	SetPosition(0, ScreenHeight - GetHeight() + 9);
	UpdateHealth();
}

void ActorLifebar::UpdateHealth()
{
	SetCrop2(glm::vec2(Health / 100, 1));
	SetWidth((Health / 100) * ScreenWidth);
}

void ActorLifebar::HitJudgement(Judgement Hit)
{
	switch (Hit)
	{
		case Excellent:
			pending_health += 3;
			break;
		case Perfect:
			pending_health += 2;
			break;
		case Great:
			pending_health += 1;
			break;
		case Bad:
			break;
		case Miss:
		case NG:
			pending_health -= 30;
	}
}

void ActorLifebar::Run(double delta)
{
	time += delta;
	if (pending_health != 0)
	{
		Health += pending_health * delta;
		pending_health -= pending_health * delta;
	}

	if (Health > 100)
		Health = 100;

	UpdateHealth();
}