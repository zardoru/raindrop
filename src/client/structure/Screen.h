#pragma once

#include "Interruptible.h"
class SceneEnvironment;

// Interface.
class Screen : public Interruptible
{
private:
    double screen_time_; // How long has it been open?
protected:

    std::shared_ptr<SceneEnvironment> scene_;

    enum EScreenState
    {
        StateIntro,
        StateRunning,
        StateExit
    }screen_state_;

    double get_screen_time() const;
    std::shared_ptr<Screen> parent_;
    bool is_active_; // Is this screen active?
    bool skip_this_frame_;

    void change_state(EScreenState new_state);
    double transition_time_;
    double intro_duration_, exit_duration_;
    std::shared_ptr<Screen> next_screen_;

public:
    explicit Screen(const std::string &name, bool init_ui = true);
    Screen(const std::string &name, const std::shared_ptr<Screen> &parent);
    virtual ~Screen();

    virtual void init();

    // Nesting screens.
    bool is_screen_running() const;
    bool run_nested(float delta);
    bool update(float delta);

    void close();

    Screen* get_top();

	void start_transition(std::shared_ptr<Screen> scr);

    // Screen implementation.
    virtual void load_resources(); // could, or not, be called from main thread.
    virtual void post_load_initialization(); // must be called from main thread - assume it always is
    virtual bool run_intro(float fraction, float delta);
    virtual bool run_exit(float fraction, float delta);
    virtual bool run(double delta) = 0;

    virtual void on_intro_begin();
    virtual void on_intro_end();
    virtual void on_running_begin();
    virtual void on_exit_begin();
    virtual void on_exit_end();

    virtual bool on_input(int32_t key, bool isPressed, bool isMouseInput);
    virtual bool on_scroll_input(double xOff, double yOff);
    virtual bool on_text_input(int codepoint);

    // We need to set up graphics again? This gets called.
    virtual void invalidate();

    // Implement this if there's anything you want to get done outside of a destructor
    // like operations that would throw exceptions.
    virtual void cleanup();
};