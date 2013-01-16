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
	origin = 1;
	position.y = PlayfieldHeight + height / 2 + ScreenOffset;
	position.x = GraphMan.GetMatrixSize().x / 2;
	Init(true);
	UpdateHealth();
}

void ActorLifebar::UpdateHealth()
{
	crop_x2 = Health / 100;
	width = (Health / 100) * PlayfieldWidth;
	Init(); // Redo our dear VBO data.
}

void ActorLifebar::HitJudgement(Judgement Hit)
{
	switch (Hit)
	{
		case Excellent:
			pending_health += 6;
			break;
		case Perfect:
			pending_health += 5;
			break;
		case Great:
			pending_health += 3;
			break;
		case Bad:
			pending_health += 1;
			break;
		case Miss:
		case NG:
			pending_health -= 15;
	}
}

void ActorLifebar::Run(float delta)
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