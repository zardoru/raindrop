#include "pch.h"

#include "GameGlobal.h"
#include "Screen.h"
#include "Audio.h"
#include "ImageLoader.h"
#include "GameWindow.h"

#include "LuaManager.h"
#include "Sprite.h"
#include "SceneEnvironment.h"
#include "ImageList.h"

#include "ScoreKeeper7K.h"
#include "ScreenGameplay7K.h"
#include "ScreenGameplay7K_Mechanics.h"

//#include <glm/gtc/matrix_transform.inl>

using namespace VSRG;

void ScreenGameplay7K::RecalculateMatrix()
{
    PositionMatrix = glm::translate(Mat4(), glm::vec3(0, JudgmentLinePos + CurrentVertical * SpeedMultiplier, 0));
}
