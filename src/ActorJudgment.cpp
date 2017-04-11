#include "pch.h"

#include "GameGlobal.h"
#include "GameState.h"
#include "Sprite.h"
#include "ActorJudgment.h"
#include "ImageLoader.h"

#define AnimDuration 0.3f

namespace Game {
	namespace dotcur {

		ActorJudgment::ActorJudgment()
		{
			Centered = Configuration::GetSkinConfigf("Centered", "Judgment") != 0;
			SetRotation(Configuration::GetSkinConfigf("Rotation", "Judgment"));
			Alpha = 0;
			SetImage(GameState::GetInstance().GetSkinImage("judge-perfect.png"));
			SetPosition(Configuration::GetSkinConfigf("X", "Judgment"), Configuration::GetSkinConfigf("Y", "Judgment"));
			AnimTime = 0;
			AffectedByLightning = true;
		}

		void ActorJudgment::ChangeJudgment(Judgment New)
		{
			AnimTime = AnimDuration;
			SetScale(1.3f);
			Alpha = 1;

			switch (New)
			{
			case J_EXCELLENT:
				SetImage(GameState::GetInstance().GetSkinImage("judge-excellent.png"));
				break;
			case J_PERFECT:
				SetImage(GameState::GetInstance().GetSkinImage("judge-perfect.png"));
				break;
			case J_GREAT:
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

		void ActorJudgment::Run(double delta)
		{
			if (AnimTime > 0)
			{
				SetScale(Vec2(LerpRatio(GetScale().x, 1.0f, AnimDuration - AnimTime, AnimDuration),
					LerpRatio(GetScale().y, 1.0f, AnimDuration - AnimTime, AnimDuration)));
			}

			AnimTime -= delta;

			if (AnimTime < -1)
			{
				Alpha -= delta; // Fade out in one second
			}
		}
	}
}