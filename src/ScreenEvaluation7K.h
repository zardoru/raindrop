#pragma once

#include "Screen.h"

class BitmapFont;
class ScoreKeeper7K;
class SceneEnvironment;

class ScreenEvaluation7K : public Screen
{
    std::string DisplayResult;
    std::shared_ptr<ScoreKeeper7K> Score;
public:
    ScreenEvaluation7K();
    void Init(std::shared_ptr<ScoreKeeper7K> Result);
    bool Run(double Delta);
    void Cleanup();
    bool HandleInput(int32_t key, KeyEventType code, bool isMouseInput);
    void PrintCLIResults(ScoreKeeper7K *result);
};
