#include "pch.h"

#include "ScreenCustom.h"
#include "SceneEnvironment.h"

ScreenCustom::ScreenCustom(const std::string& ScriptName)
    : Screen("ScreenCustom")
{
    Animations->Initialize(ScriptName);
    IntroDuration = Animations->GetIntroDuration();
    ExitDuration = Animations->GetExitDuration();
    ChangeState(StateIntro);
    Running = true;
}

bool ScreenCustom::Run(double Delta)
{
    // Update and draw targets, and carry on.
    Animations->DrawTargets(Delta);
    return true;
}

bool ScreenCustom::HandleInput(int32_t key, KeyEventType code, bool isMouseInput)
{
    return Animations->HandleInput(key, code, isMouseInput);
}