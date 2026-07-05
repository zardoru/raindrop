#include <memory>
#include <string>
#include <rmath.h>
#include <map>
#include <functional>

#include <filesystem>

#include <game/GameConstants.h>
#include "../game/PlayscreenParameters.h"
#include "../game/GameState.h"
#include "SceneEnvironment.h"
#include "Screen.h"

Screen::Screen(std::string Name, bool InitUI)
{
    Parent = nullptr;
    is_active_ = false;
    Next = nullptr;
    ScreenTime = 0;
    IntroDuration = 0;
    ScreenState = StateRunning;
    scene_ = std::make_shared<SceneEnvironment>(Name.c_str(), InitUI);
    SkipThisFrame = true;
}

Screen::Screen(std::string Name, std::shared_ptr<Screen> _Parent)
{
    Parent = _Parent;
    is_active_ = false;
    Next = 0;
    ScreenTime = 0;
    IntroDuration = 0;
    ScreenState = StateRunning;
    scene_ = std::make_shared<SceneEnvironment>(Name.c_str());
    SkipThisFrame = true;
}

Screen::~Screen() {}

bool Screen::on_text_input(int codepoint)
{
    if (Next)
        return Next->on_text_input(codepoint);
    return scene_->handle_text_input(codepoint);
}

void Screen::Close()
{
    cleanup();
    is_active_ = false;
    if (Next)
        Next->Close();
}

void Screen::load_resources()
{
    // virtual
}

void Screen::post_load_initialization()
{
    // virtual
}

void Screen::ChangeState(Screen::EScreenState NewState)
{
    ScreenState = NewState;

    switch (NewState)
    {
    case StateIntro:
        OnIntroBegin();
        break;
    case StateExit:
        OnExitBegin();
        break;
    default:
        break;
    }

    TransitionTime = 0;
    SkipThisFrame = true;
}

bool Screen::IsScreenRunning()
{
    return is_active_;
}

bool Screen::RunNested(float delta)
{
    if (!Next)
        return false;

    if (Next->update(delta))
        return true;
    else // The screen's done?
    {
        // It's not null- so we'll delete it.
        Next->cleanup();
        Next = nullptr;
        return false;
    }

    // Reaching this point SHOULDn't happen.
    return false;
}

Screen* Screen::GetTop()
{
    if (Next) return Next->GetTop();
    else return this;
}

void Screen::StartTransition(std::shared_ptr<Screen> scr)
{
	Next = scr;
}

double Screen::GetScreenTime()
{
    return ScreenTime;
}

bool Screen::update(float delta)
{
    ScreenTime += delta;

    if (SkipThisFrame)
    {
        SkipThisFrame = false;
        return true;
    }

    if (ScreenState == StateIntro)
    {
        float Frac;
        TransitionTime += delta;

        if (TransitionTime < IntroDuration)
            Frac = clamp(TransitionTime / IntroDuration, 0.0, 1.0);
        else
        {
            Frac = 1;
            ScreenState = StateRunning;
        }

        return RunIntro(Frac, delta);
    }
    else if (ScreenState == StateExit)
    {
        float Frac;

        TransitionTime += delta;
        if (TransitionTime < ExitDuration)
            Frac = clamp(TransitionTime / ExitDuration, 0.0, 1.0);
        else
        {
            Frac = 1;
            ScreenState = StateRunning;
        }

        /*
            StateExit can still go back to the "StateRunning" state
            This way it can be used for transitions.
        */
        return RunExit(Frac, delta);
    }
    else
        return Run(delta);
}

void Screen::Init()
{
    load_resources();
    post_load_initialization();
}

bool Screen::RunIntro(float Fraction, float Delta)
{
    scene_->RunIntro(Fraction, Delta);

    if (Fraction == 1)
        OnIntroEnd();

    return is_active_;
}

bool Screen::RunExit(float Fraction, float Delta)
{
    scene_->RunExit(Fraction, Delta);

    if (Fraction == 1)
        OnExitEnd();

    return is_active_;
}
bool Screen::on_input(int32_t key, bool isPressed, bool isMouseInput)
{
    if (Next && Next->IsScreenRunning())
        return Next->on_input(key, isPressed, isMouseInput);

    return false;
}

bool Screen::on_scroll_input(double xOff, double yOff)
{
    if (Next && Next->IsScreenRunning())
        return Next->on_scroll_input(xOff, yOff);

    return false;
}
void Screen::OnIntroBegin()
{
    scene_->trigger_event("OnIntroBegin");
}

void Screen::OnIntroEnd()
{
    scene_->trigger_event("OnIntroEnd");
}

void Screen::OnExitBegin()
{
    scene_->trigger_event("OnExitBegin");
}

void Screen::OnExitEnd()
{
    scene_->trigger_event("OnExitEnd");

	if (!Next)
		Next = GameState::get_instance().get_next_screen();
}

void Screen::OnRunningBegin()
{
    scene_->trigger_event("OnRunningBegin");
}

void Screen::cleanup() { /* stub */ }

void Screen::Invalidate() { /* stub */ }