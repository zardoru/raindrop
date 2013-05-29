#include "Global.h"
#include "Game_Consts.h"
#include "GraphObject2D.h"
#include "ActorLifebar.h"
#include "ImageLoader.h"
#include "GraphicsManager.h"

ActorLifebar::ActorLifebar()
{
	Health = 50; // Out of 100!
	pending_health = 0;
	setImage(ImageLoader::LoadSkin("healthbar.png"));
	red = blue = 0;
	time = 0;
	Centered = false;
	position.y = ScreenHeight - height;
	position.x = 0;
	InitTexture();
	UpdateHealth();
}

void ActorLifebar::UpdateHealth()
{
	crop_x2 = Health / 100;
	width = (Health / 100) * ScreenWidth;
	UpdateTexture(); // Redo our dear VBO data.
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