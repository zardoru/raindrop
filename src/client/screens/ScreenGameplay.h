#pragma once

#include <ChartGroup.h>
#include <game/TimingWindows.h>

class AudioStream;
class Texture2D;
class SceneEnvironment;
class LuaManager;

class ScreenGameplay : public Screen
{
private:


    std::map <int, std::vector<std::shared_ptr<AudioSample> > > keysounds_;
    std::vector<std::unique_ptr<PlayerContext>> players_;
    std::map<int, bool> playfield_clip_enabled_;
    std::map<int, AABB> playfield_clip_area_;

    std::queue<otoworm::AutoplaySound>   bgm_events_;

    std::shared_ptr<otoworm::ChartGroup> my_chart_group_;
    std::shared_ptr<otoworm::ChartGroup> loaded_chart_group_;

    struct {
        double stream;
        double old_stream; // Previous frame's stream song time
        double waiting; // Time before T = 0
        double game; // Overall screen time
        double miss_layer; // Time for showing MISS layer
        double failure; // Time for showing Failure state
        double success; // Time for showing Success state
        double audio_old; // DAC thread start time and previous DAC time
    } time_;

    struct {
        double ToleranceMS;
        double AudioDrift;
    } TimeError;


    int				 start_measure_;

    std::unique_ptr<AudioStream> music_;
    std::unique_ptr<AudioSourceOJM> ojm_audio_;
    AudioSample miss_snd_;
    AudioSample fail_snd_;

    /* Effects */
    bool stage_failure_triggered_;
    bool active_;
    bool start_active_;
    bool load_successful_;
    bool play_reactive_sounds_;
    bool song_pass_triggered_;

    std::unique_ptr<BackgroundAnimation> bga_;
    void register_script_values() const;

    // Done in loading thread
    bool load_chart_data();
    bool load_song_audio();
    void load_samples();
    void load_bmson();
    bool load_bga() const;
    bool process_song();

    void add_script_classes(LuaManager* Env);


    void jump_to_measure(uint32_t measure);
    void run_auto_events();
    void evaluate_stage_failure();
    bool has_delayed_failure() const;
    bool all_players_failed() const;
    bool has_song_finished() const;

    void update_song_time(float delta);
    void on_player_hit(rd::ScoreKeeperJudgment judgment, double dt, uint32_t lane, rd::NoteJudgmentPart part, int pn) const;
    void render();

    void play_keysound(int keysound);

    void activate();


    void on_player_miss(double dt, uint32_t lane, bool hold, bool dontbreakcombo, bool earlymiss, int pn) const;

    void on_player_gear_key_event(uint32_t lane, bool keydown, int pn) const;

    void set_player_clip(int pn, AABB box);
    void disable_player_clip(int pn);

    friend class Noteskin;
public:

    void setup_scripts(LuaManager* Env);

    // Functions for data.
    bool is_active() const;
    otoworm::ChartGroup* get_chart_group() const;


    explicit ScreenGameplay(GameWindow& window);
    void initialize(std::shared_ptr<otoworm::ChartGroup> chart_group);
    void load_resources() override;
    void post_load_initialization() override;

    void cleanup() override;

    PlayerContext* GetPlayerContext(int i) const;

    bool run(double delta) override;
    bool on_input(int32_t key, bool isPressed, bool isMouseInput) override;
};
