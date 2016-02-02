#include "pch.h"

#include "GameGlobal.h"
#include "GameState.h"
#include "Sprite.h"
#include "BitmapFont.h"
#include "ScreenEvaluation.h"
#include "GameWindow.h"
#include "ImageLoader.h"
#include "Audio.h"

AudioStream *ScreenEvaluationMusic = NULL;

ScreenEvaluation::ScreenEvaluation() :
    Screen("ScreenEvaluation", nullptr)
{
    Running = true;
    Font = NULL;
}

int32_t ScreenEvaluation::CalculateScore()
{
    return int32_t(1000000.0 * Results.dpScoreSquare / (double)(Results.totalNotes * (Results.totalNotes + 1)));
}

void ScreenEvaluation::Init(EvaluationData _Data, std::string SongAuthor, std::string SongTitle)
{
    if (!ScreenEvaluationMusic)
    {
        ScreenEvaluationMusic = new SoundStream();
        ScreenEvaluationMusic->Open((GameState::GetInstance().GetSkinFile("screenevaluationloop.ogg")).c_str());
        ScreenEvaluationMusic->SetLoop(true);
    }

    ScreenEvaluationMusic->SeekTime(0);
    ScreenEvaluationMusic->Play();

    Background.SetImage(GameState::GetInstance().GetSkinImage(Configuration::GetSkinConfigs("EvaluationBackground")));
    Background.AffectedByLightning = true;
    if (!Font)
    {
        Font = new BitmapFont();
        Font->LoadSkinFontImage("font_screenevaluation.tga", Vec2(10, 20), Vec2(32, 32), Vec2(10, 20), 32);
        Font->SetAffectedByLightning(true);
    }
    Results = _Data;

    char _Results[256];
    const char *Text = "Excellent: \n"
        "Perfect:   \n"
        "Great:     \n"
        "Bad:       \n"
        "NG:        \n"
        "OK:        \n"
        "Misses:    \n"
        "\n"
        "Max Combo: \n"
        "Score:     \n";

    ResultsGString = Text;

    sprintf(_Results, "%d\n%d\n%d\n%d\n%d\n%d\n%d\n\n%d\n%d\n",
        Results.NumExcellents,
        Results.NumPerfects,
        Results.NumGreats,
        Results.NumBads,
        Results.NumNG,
        Results.NumOK,
        Results.NumMisses,
        Results.MaxCombo,
        CalculateScore());

    ResultsNumerical = _Results;
    WindowFrame.SetLightMultiplier(1);
    WindowFrame.SetLightPosition(glm::vec3(0, 0, 1));

    TitleFormat = SongTitle + " by " + SongAuthor;
}

bool ScreenEvaluation::HandleInput(int32_t key, KeyEventType code, bool isMouseInput)
{
    if ((BindingsManager::TranslateKey(key) == KT_Escape || BindingsManager::TranslateKey(key) == KT_Select) && code == KE_PRESS)
        Running = false;

    return true;
}

void ScreenEvaluation::Cleanup()
{
    ScreenEvaluationMusic->Stop();
    delete Font;
}

bool ScreenEvaluation::Run(double Delta)
{
    WindowFrame.SetLightMultiplier(sin(GetScreenTime()) * 0.2 + 1);

    Background.Render();
    if (Font)
    {
        Font->Render(ResultsGString, Vec2(ScreenWidth / 2 - 110, ScreenHeight / 2 - 100));
        Font->Render(ResultsNumerical, Vec2(ScreenWidth / 2, ScreenHeight / 2 - 100));
        Font->Render(std::string("results screen"), Vec2(ScreenWidth / 2 - 70, 0));
        Font->Render(std::string("press space to continue..."), Vec2(ScreenWidth / 2 - 130, ScreenHeight * 7 / 8));
        Font->Render(TitleFormat, Vec2(0, ScreenHeight - 20));
    }
    return Running;
}