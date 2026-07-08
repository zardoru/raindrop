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

Screen::Screen(const std::string &name, bool init_ui) : transition_time_(0), exit_duration_(0) {
    parent_ = nullptr;
    is_active_ = false;
    next_screen_ = nullptr;
    screen_time_ = 0;
    intro_duration_ = 0;
    screen_state_ = StateRunning;
    scene_ = std::make_shared<SceneEnvironment>(name.c_str(), init_ui);
    skip_this_frame_ = true;
}

Screen::Screen(const std::string &name, const std::shared_ptr<Screen> &parent)
{
    parent_ = parent;
    is_active_ = false;
    next_screen_ = 0;
    screen_time_ = 0;
    intro_duration_ = 0;
    screen_state_ = StateRunning;
    scene_ = std::make_shared<SceneEnvironment>(name.c_str());
    skip_this_frame_ = true;
}

Screen::~Screen() {}

bool Screen::on_text_input(int codepoint)
{
    if (next_screen_)
        return next_screen_->on_text_input(codepoint);
    return scene_->handle_text_input(codepoint);
}

void Screen::close()
{
    cleanup();
    is_active_ = false;
    if (next_screen_)
        next_screen_->close();
}

void Screen::load_resources()
{
    // virtual
}

void Screen::post_load_initialization()
{
    // virtual
}

void Screen::change_state(Screen::EScreenState new_state)
{
    screen_state_ = new_state;

    switch (new_state)
    {
    case StateIntro:
        on_intro_begin();
        break;
    case StateExit:
        on_exit_begin();
        break;
    default:
        break;
    }

    transition_time_ = 0;
    skip_this_frame_ = true;
}

bool Screen::is_screen_running() const {
    return is_active_;
}

bool Screen::run_nested(float delta)
{
    if (!next_screen_)
        return false;

    if (next_screen_->update(delta))
        return true;
    else // The screen's done?
    {
        // It's not null- so we'll delete it.
        next_screen_->cleanup();
        next_screen_ = nullptr;
        return false;
    }

    // Reaching this point SHOULDn't happen.
    return false;
}

Screen* Screen::get_top()
{
    if (next_screen_) return next_screen_->get_top();
    else return this;
}

void Screen::start_transition(std::shared_ptr<Screen> scr)
{
	next_screen_ = scr;
}

double Screen::get_screen_time() const {
    return screen_time_;
}

bool Screen::update(float delta)
{
    screen_time_ += delta;

    if (skip_this_frame_)
    {
        skip_this_frame_ = false;
        return true;
    }

    if (screen_state_ == StateIntro)
    {
        float frac;
        transition_time_ += delta;

        if (transition_time_ < intro_duration_)
            frac = clamp(transition_time_ / intro_duration_, 0.0, 1.0);
        else
        {
            frac = 1;
            screen_state_ = StateRunning;
        }

        return run_intro(frac, delta);
    }
    else if (screen_state_ == StateExit)
    {
        float frac;

        transition_time_ += delta;
        if (transition_time_ < exit_duration_)
            frac = clamp(transition_time_ / exit_duration_, 0.0, 1.0);
        else
        {
            frac = 1;
            screen_state_ = StateRunning;
        }

        /*
            StateExit can still go back to the "StateRunning" state
            This way it can be used for transitions.
        */
        return run_exit(frac, delta);
    }
    else
        return run(delta);
}

void Screen::init()
{
    load_resources();
    post_load_initialization();
}

bool Screen::run_intro(const float fraction, const float delta)
{
    scene_->RunIntro(fraction, delta);

    if (fraction == 1)
        on_intro_end();

    return is_active_;
}

bool Screen::run_exit(const float fraction, const float delta)
{
    scene_->RunExit(fraction, delta);

    if (fraction == 1)
        on_exit_end();

    return is_active_;
}
bool Screen::on_input(int32_t key, bool isPressed, bool isMouseInput)
{
    if (next_screen_ && next_screen_->is_screen_running())
        return next_screen_->on_input(key, isPressed, isMouseInput);

    return false;
}

bool Screen::on_scroll_input(double xOff, double yOff)
{
    if (next_screen_ && next_screen_->is_screen_running())
        return next_screen_->on_scroll_input(xOff, yOff);

    return false;
}
void Screen::on_intro_begin()
{
    scene_->trigger_event("OnIntroBegin");
}

void Screen::on_intro_end()
{
    scene_->trigger_event("OnIntroEnd");
}

void Screen::on_exit_begin()
{
    scene_->trigger_event("OnExitBegin");
}

void Screen::on_exit_end()
{
    scene_->trigger_event("OnExitEnd");

	if (!next_screen_)
		next_screen_ = GameState::get_instance().get_next_screen();
}

void Screen::on_running_begin()
{
    scene_->trigger_event("OnRunningBegin");
}

void Screen::cleanup() { /* stub */ }

void Screen::invalidate() { /* stub */ }