#pragma once

class BitmapFont;

namespace otoworm {
    class ChartGroup;
}

class SceneEnvironment;

class AudioStream;

class AudioSample;

class ScreenSelectMusic : public Screen {
    double Time;
    double TransitionTime;
    double PreviewWaitTime;

    std::shared_ptr<otoworm::ChartGroup> to_preview;
    std::shared_ptr<otoworm::ChartGroup> previous_preview;

    std::shared_ptr<AudioStream> PreviewStream;

    bool SwitchBackGuiPending;
    bool IsTransitioning;

    void PlayPreview();

    void PlayLoops();

    void StopLoops();

    std::unique_ptr<AudioStream> BGM;
    std::unique_ptr<AudioSample> SelectSnd;
    std::unique_ptr<AudioSample> ClickSnd;


    void StartGameplayScreen();

    float GetListVerticalTransformation(const float Y);

    float GetListHorizontalTransformation(const float Y);

    float GetListWidthTransformation(const float Y);

    float GetListHeightTransformation(const float Y);

    void OnSongChange(std::shared_ptr<otoworm::ChartGroup> chart_group, uint8_t difindex);

    void OnSongSelect(std::shared_ptr<otoworm::ChartGroup> chart_group, uint8_t difindex);

    void OnDirectoryChange();

    void OnItemClick(int32_t index, uint32_t bound_index, std::string Line, std::shared_ptr<otoworm::ChartGroup> selected);

    void OnItemHover(int32_t index, uint32_t bound_index, std::string Line, std::shared_ptr<otoworm::ChartGroup> selected);

    void OnItemHoverLeave(int32_t index, uint32_t bound_index, std::string Line, std::shared_ptr<otoworm::ChartGroup> selected);

    void TransformItem(int Item, std::shared_ptr<otoworm::ChartGroup> chart_group, bool IsSelected, int ListItem);

    void TransformString(int Item, std::shared_ptr<otoworm::ChartGroup> chart_group, bool IsSelected, int ListItem, std::string text);

public:
    ScreenSelectMusic();

    void load_resources() override;

    void post_load_initialization() override;

    bool Run(double Delta) override;

    void Cleanup() override;

    float GetTransform(const char *TransformName, const float Y);

    bool HandleInput(int32_t key, bool isPressed, bool isMouseInput) override;

    bool HandleScrollInput(double xOff, double yOff) override;
};
