#include <filesystem>
#include <functional>

#include "Screen.h"
#include "ScreenCustom.h"
#include "SceneEnvironment.h"

ScreenCustom::ScreenCustom(const std::filesystem::path& ScriptName)
    : Screen("ScreenCustom", false)
{
    Animations->Initialize(ScriptName);
    IntroDuration = Animations->GetIntroDuration();
    ExitDuration = Animations->GetExitDuration();
    ChangeState(StateIntro);
    Running = true;

	Animations->SetScreenName(ScriptName.filename().replace_extension().string());
	Animations->InitializeUI();
}

bool ScreenCustom::Run(double Delta)
{
    // Update and draw targets, and carry on.
    Animations->DrawTargets(Delta);
    return true;
}

bool ScreenCustom::HandleInput(int32_t key, bool isPressed, bool isMouseInput)
{
    return Animations->HandleInput(key, isPressed, isMouseInput);
}