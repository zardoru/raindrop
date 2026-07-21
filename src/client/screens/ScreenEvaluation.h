#pragma once

#include "../structure/Screen.h"

class BitmapFont;

class ScoreKeeper;

class SceneEnvironment;

class ScreenGameplay;

class ScreenEvaluation : public Screen {
public:
    explicit ScreenEvaluation(GameWindow& window);

    void init(ScreenGameplay *rs);

    bool run(double Delta);

    void cleanup();

    bool on_input(int32_t key, bool isPressed, bool isMouseInput);

    void PrintCLIResults(ScoreKeeper *result);
};

