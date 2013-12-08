#include "Global.h"
#include "Game_Consts.h"
#include "GraphObject2D.h"
#include "ActorLifebar.h"
#include "ImageLoader.h"
#include "GameWindow.h"

ActorLifebar::ActorLifebar() : GraphObject2D()
{
	Health = 50; // Out of 100!
	pending_health = 0;
	SetImage(ImageLoader::LoadSkin("healthbar.png"));
	SetWidth(PlayfieldHeight / 2);
	time = 0;
	Centered = true;
	SetPosition(ScreenWidth - GetHeight()/2, ScreenHeight/2);
	UpdateHealth();
	SetRotation(90);
	AffectedByLightning = true;
}

void ActorLifebar::UpdateHealth()
{
	if (Health < 0)
	{
		Health = 0;
	}

	SetCrop2(glm::vec2(Health / 100, 1));
	Red = Green = Blue = Health / 100;
	SetWidth((Health / 100) * PlayfieldHeight);
}

void ActorLifebar::HitJudgement(Judgement Hit)
{
	switch (Hit)
	{
		case Excellent:
			pending_health += 2;
			break;
		case Perfect:
			pending_health += 1;
			break;
		case Great:
			break;
		case Bad:
			pending_health -= 3;
			break;
		case Miss:
		case NG:
			pending_health -= 20;
	}
}

void ActorLifebar::Run(double delta)
{
	time += delta;
	if (pending_health != 0)
	{
		Health += pending_health * delta;
		pending_health -= pending_health * delta;

		if (pending_health > 400) // only up to 4x health
			pending_health = 400;

		/* Accomulate health. Better you do, more forviging to mistakes. */
		if (Health > 100)
		{
			pending_health += Health - 100;
			Health = 100;
		}
	}

	UpdateHealth();
}