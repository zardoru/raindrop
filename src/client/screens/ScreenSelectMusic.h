#pragma once

class BitmapFont;

namespace otoworm {
    class ChartGroup;
}

class SceneEnvironment;

class AudioStream;

class AudioSample;

class ScreenSelectMusic : public Screen {
    double time_;
    double transition_time_;
    double preview_wait_time_;

    std::shared_ptr<otoworm::ChartGroup> to_preview;
    std::shared_ptr<otoworm::ChartGroup> previous_preview;

    std::shared_ptr<AudioStream> preview_stream_;


    std::unique_ptr<AudioStream> bgm_;
    std::unique_ptr<AudioSample> select_snd_;
    std::unique_ptr<AudioSample> click_snd_;

    bool switch_back_gui_pending_;
    bool is_transitioning_;

    void play_preview();

    void play_loops();

    void stop_loops();



    void start_gameplay_screen();

    float get_list_vertical_transformation(const float Y) const;

    float get_list_horizontal_transformation(const float Y) const;

    float get_list_width_transformation(const float Y) const;

    float get_list_height_transformation(const float Y) const;

    void on_song_change(std::shared_ptr<otoworm::ChartGroup> chart_group, uint8_t difindex);

    void on_song_select(std::shared_ptr<otoworm::ChartGroup> chart_group, uint8_t difindex);

    void on_directory_change() const;

    void on_item_click(int32_t index, uint32_t bound_index, std::string Line, std::shared_ptr<otoworm::ChartGroup> selected) const;

    void on_item_hover(int32_t index, uint32_t bound_index, std::string Line, std::shared_ptr<otoworm::ChartGroup> selected) const;

    void on_item_hover_leave(int32_t index, uint32_t bound_index, std::string Line, std::shared_ptr<otoworm::ChartGroup> selected) const;

    void transform_item(int Item, std::shared_ptr<otoworm::ChartGroup> chart_group, bool IsSelected, int ListItem) const;

    void transform_string(int Item, std::shared_ptr<otoworm::ChartGroup> chart_group, bool IsSelected, int ListItem, std::string text) const;

public:
    explicit ScreenSelectMusic(GameWindow& window);

    void load_resources() override;

    void post_load_initialization() override;

    bool run(double delta) override;

    void cleanup() override;

    float get_transform(const char *transform_name, const float Y) const;

    bool on_input(int32_t key, bool isPressed, bool isMouseInput) override;

    bool on_scroll_input(double xOff, double yOff) override;
};
