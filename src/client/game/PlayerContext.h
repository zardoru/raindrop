#pragma once

#include <game/RaindropProcessedChart.h>
#include <ChartGroup.h>

class Line;
class Noteskin;
class Replay;

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
    rd::RaindropProcessedChart ChartState;

    double LastUpdateTime; // seconds, song time

    std::shared_ptr<otoworm::Chart> CurrentChart;

    std::unique_ptr<rd::Mechanics> MechanicsSet;
    std::shared_ptr<rd::ScoreKeeper> PlayerScoreKeeper;

    Line* Barline;

    double MsDisplayMargin;
    double Drift, JudgeOffset;
    double UnitsPerMeasure;
    bool JudgeNotes;

    PlayscreenParameters Parameters;
    Replay* PlayerReplay;

    struct SGearState {
        std::map<int, int> Bindings;
        rd::RuntimeNote*   CurrentKeysounds[rd::MAX_CHANNELS];
        bool IsPressed[rd::MAX_CHANNELS]; //  Whether the lane is pressed
        bool HeldKey[rd::MAX_CHANNELS]; //  Whether a hold note is active
        int  ClosestNoteMS[rd::MAX_CHANNELS];
        /*
            HeldKey is only active if there's a hold right now.
            IsPressed is active any time the key for that lane is down.
        */
        bool TurntableEnabled;
    } Gear;

    void DrawBarlines(double cur_vertical, double smult) const;
    int DrawMeasures(double song_time); // returns rendered note count

    Noteskin* PlayerNoteskin;
    int PlayerNumber;

    void SetupMechanics();
    void RunMeasures(double time);
    void play_lane_keysound(uint32_t Lane) const;
    void RunAuto(rd::RuntimeNote *m, double usedTime, uint32_t k);

    void on_player_key_event(double Time, bool KeyDown, uint32_t lane);
public:
    PlayerContext(int pn, PlayscreenParameters par = PlayscreenParameters());
    ~PlayerContext();
    void init() const;
    void validate();
    void update(double songTime);
    void render(double songTime);

    std::function<void(int sndid)> PlayKeysound;
    std::function<void(rd::ScoreKeeperJudgment judgment, double dt, uint32_t lane, bool hold, bool release, int pn)> OnHit;
    std::function<void(double dt, uint32_t lane, bool hold, bool dontbreakcombo, bool earlymiss, int pn)> OnMiss;
    std::function<void(uint32_t lane, bool keydown, int pn)> OnGearKeyEvent;

    /*
        About this pointer's lifetime:
        PlayerContext requires the song/difficulty pointer to stay valid until it's destroyed.
    */
    void set_playable_data(std::shared_ptr<otoworm::Chart> chart, double Drift = 0);
    const rd::RaindropProcessedChart &get_chart_state();

    // Getters (Lua)
    bool is_fail_enabled() const;
    bool is_upscrolling() const;
    bool get_uses_turntable() const;

    double GetAppliedSpeedMultiplier(double Time) const;
    double get_current_beat() const;
    double get_user_multiplier() const;
    double get_current_vertical_speed() const;
    double get_warped_song_time() const;
    double GetCurrentBPM() const;
    double GetJudgmentY() const;
    double get_life_pst() const;
    std::string GetPacemakerText(bool bm) const;
    int GetPacemakerValue(bool bm) const;

    double GetChartTimeAt(double time) const;
    int GetChannelCount() const;
    int get_player_number() const;
    bool get_is_held_key(int Lane) const;
    double has_song_finished(double time) const;

    double get_waiting_time() const;

    otoworm::Chart* get_chart() const;

    double get_duration() const;
    double get_beat_duration() const;
    /*
        "So why is this returning a raw pointer?"
        Lua binding. The answer is lua binding.
    */
    rd::ScoreKeeper* GetScoreKeeper() const;

    std::shared_ptr<rd::ScoreKeeper> GetScoreKeeperShared() const;

    double GetClosestNoteTime(int Lane) const;

    // Setters
    void SetUserMultiplier(float Multip);

    // Only if Difficulty->Data is not null.
    std::vector<otoworm::AutoplaySound> GetBgmData();

    static void SetupLua(LuaManager *Env);

    Replay get_replay() const;
    void load_replay(std::filesystem::path path) const;

    double GetScore() const;
    int GetCombo() const;

    void hit_note(double TimeOff, uint32_t Lane, bool IsHold, bool IsHoldRelease = false) const;
    void miss_note(double TimeOff, uint32_t Lane, bool IsHold, bool dont_break_combo, bool early_miss);
    void GearKeyEvent(uint32_t Lane, bool KeyDown) const;
    void judge_lane(uint32_t Lane, double Time);
    void release_lane(uint32_t Lane, double Time);
    void translate_key(int32_t K, bool KeyDown, double Time);
    void set_lane_hold_state(uint32_t Lane, bool NewState);
    // true if holding down key
    bool get_gear_lane_state(uint32_t Lane) const;
    bool BindKeysToLanes(bool UseTurntable);

    void SetCanJudge(bool canjudge);
    bool CanJudge();

    void SetUnwarpedTime(double time);

    int GetCurrentGaugeType() const;
    int get_current_score_type() const;
    int get_current_system_type() const;

    // in seconds - chart time displacement
    double get_drift() const;

    // in seconds - key event judge time displacement
    double get_judge_offset() const;

    double GetRate() const;

    // Whether the player has actually failed or not
    bool has_failed() const;

    // Whether failure is delayed until the screen is over
    bool has_delayed_failure() const;
};
