#pragma once

#include <game/RaindropProcessedChart.h>
#include <ChartGroup.h>

class Line;
class Noteskin;
class Replay;
class DrawCallSink;

/*
	Usage of a PlayerContext requires several steps.
	* If you're using a SceneEnvironment, register the context's type via SetupLua.
	* Setup of playable data, along with player parameters.
	* The difficulty object must have the "Data" structure non-null.
	After that the difficulty's data can be released.
	
	(optional unless you want to display stuff)
	* Then, that the callbacks for keysound stuff is set up 
		* OnMiss, OnHit, PlayKeysound are the required functions.

	* A SceneEnvironment must be active so that it can send the PC data to it on private events.
	* At this point you can Validate() and mechanics and stuff will be set up internally for legit usage.

*/

namespace rd {
    class ScoreKeeper;
    class Mechanics;
}

/* fixme: add constructors and destructors initialization and destruction of these new pointers */
class PlayerContext {
public:
    ;
private:
    rd::RaindropProcessedChart chart_state_;

    double last_update_time_; // seconds, song time

    std::shared_ptr<otoworm::Chart> current_chart_;

    std::unique_ptr<rd::Mechanics> mechanics_set_;
    std::shared_ptr<rd::ScoreKeeper> player_score_keeper_;

    std::unique_ptr<Line> barline_;

    double ms_display_margin_;
    double drift_, judge_offset_;
    double units_per_measure_;
    bool judge_notes_;

    PlayscreenParameters parameters_;
    std::shared_ptr<Replay> replay_data_;

    struct SGearState {
        std::map<int, int> key_to_lane_bindings;
        rd::RuntimeNote*   current_keysounds[rd::MAX_CHANNELS];
        int  closest_note_timedist_ms[rd::MAX_CHANNELS];
        bool is_key_down[rd::MAX_CHANNELS]; //  Whether the lane is pressed
        bool is_hold_active[rd::MAX_CHANNELS]; //  Whether a hold note is active
        /*
            HeldKey is only active if there's a hold right now.
            IsPressed is active any time the key for that lane is down.
        */
        bool has_turntable_enabled;
    } gear_state_;

    void draw_barlines(double cur_vertical, double user_speed_multiplier) const;
    int draw_measures(double song_time); // returns rendered note count

    std::unique_ptr<Noteskin> noteskin_;
    DrawCallSink *draw_calls_ = nullptr;
    int player_number_;

    void setup_mechanics();
    void run_measures(double time);
    void play_lane_keysound(uint32_t Lane) const;
    void run_autoplay(rd::RuntimeNote *m, double usedTime, uint32_t k);

    void on_player_key_event(double time, bool key_down, uint32_t lane);
public:
    PlayerContext(int pn, PlayscreenParameters par = PlayscreenParameters());
    ~PlayerContext();
    void init() const;
    void finalize_loading();
    void update(double song_time);
    void emit_draw_calls(double song_time, DrawCallSink &sink);

    std::function<void(int sndid)> play_keysound;
    std::function<void(rd::ScoreKeeperJudgment judgment, double dt, uint32_t lane, bool hold, bool release, int pn)> on_hit;
    std::function<void(double dt, uint32_t lane, bool hold, bool dontbreakcombo, bool earlymiss, int pn)> on_miss;
    std::function<void(uint32_t lane, bool keydown, int pn)> on_gear_key_event;

    /*
        About this pointer's lifetime:
        PlayerContext requires the song/difficulty pointer to stay valid until it's destroyed.
    */
    void set_playable_data(std::shared_ptr<otoworm::Chart> chart, double drift = 0);
    const rd::RaindropProcessedChart &get_chart_state();

    // Getters (Lua)
    bool is_fail_enabled() const;
    bool is_autoplay() const;
    bool is_upscrolling() const;
    bool get_uses_turntable() const;

    double get_applied_speed_multiplier(double time) const;
    double get_current_beat() const;
    double get_user_multiplier() const;
    double get_current_vertical_speed() const;
    double get_warped_song_time() const;
    double get_current_bpm() const;
    double get_judgment_y() const;
    double get_life_pst() const;
    std::string get_pacemaker_text(bool bm) const;
    int get_pacemaker_value(bool bm) const;

    double get_chart_time_at(double time) const;
    int get_channel_count() const;
    int get_player_number() const;
    bool get_is_held_key(int Lane) const;
    bool has_song_finished(double time) const;

    double get_waiting_time() const;

    otoworm::Chart* get_chart() const;

    double get_duration() const;
    double get_beat_duration() const;
    /*
        "So why is this returning a raw pointer?"
        Lua binding. The answer is lua binding.
    */
    rd::ScoreKeeper* get_score_keeper() const;

    std::shared_ptr<rd::ScoreKeeper> get_score_keeper_shared() const;

    double get_closest_note_time(int lane) const;

    // Setters
    void set_user_multiplier(float multip);

    // Only if Difficulty->Data is not null.
    std::vector<otoworm::AutoplaySound> create_autoplay_sound_list();

    static void setup_script_context(LuaManager *scripts);

    Replay get_replay() const;
    void load_replay(const std::filesystem::path &path) const;

    double get_score() const;
    int get_combo() const;

    void hit_note(double time_off, uint32_t lane, bool is_hold, bool is_hold_release = false) const;
    void miss_note(double time_off, uint32_t lane, bool is_hold, bool dont_break_combo, bool early_miss);
    void gear_key_event(uint32_t lane, bool key_down) const;
    void judge_lane(uint32_t lane, double Time);
    void release_lane(uint32_t Lane, double Time);
    void handle_lane_events(int32_t key, bool key_down, double time);
    void set_lane_hold_state(uint32_t Lane, bool NewState);
    // true if holding down key
    bool get_gear_lane_state(uint32_t Lane) const;
    bool bind_keys_to_lanes(bool use_turntable);

    void set_can_judge(bool can_judge);
    bool can_judge() const;

    void set_unwarped_time(double time);

    int get_current_gauge_type() const;
    int get_current_score_type() const;
    int get_current_system_type() const;

    // in seconds - chart time displacement
    double get_drift() const;

    // in seconds - key event judge time displacement
    double get_judge_offset() const;

    double get_rate() const;

    // Whether the player has actually failed or not
    bool has_failed() const;

    // Whether failure is delayed until the screen is over
    bool has_delayed_failure() const;
};
