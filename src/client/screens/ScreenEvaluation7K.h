#pragma once

#include "../structure/Screen.h"

class BitmapFont;

class ScoreKeeper;

class SceneEnvironment;

class ScreenGameplay;

class ScreenEvaluation : public Screen {
public:
    ScreenEvaluation();

    void Init(ScreenGameplay *rs);

    bool Run(double Delta);

    void Cleanup();

    bool HandleInput(int32_t key, bool isPressed, bool isMouseInput);

    void PrintCLIResults(ScoreKeeper *result);
};


