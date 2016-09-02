#pragma once

class BitmapFont;

namespace Game {
	namespace dotcur
	{
		class Song;
	}

	namespace VSRG
	{
		class Song;
	}
}

namespace GUI
{
    class Button;
}

class SceneEnvironment;
class AudioStream;

class ScreenSelectMusic : public Screen
{
    double Time;
    double TransitionTime;
    double PreviewWaitTime;

    std::shared_ptr<Game::Song> ToPreview;
    std::shared_ptr<Game::Song> PreviousPreview;
    Sprite Background;

	std::unique_ptr<GUI::Button>
		UpBtn, BackBtn;

    std::shared_ptr<AudioStream> PreviewStream;

    bool SwitchBackGuiPending;
    bool IsTransitioning;

    void PlayPreview();
    void PlayLoops();
    void StopLoops();

	std::unique_ptr<SoundStream> BGM;
	std::unique_ptr<SoundSample> SelectSnd;
	std::unique_ptr<SoundSample> ClickSnd;


    float GetListHorizontalTransformation(const float Y);
    void StartGameplayScreen();
    float GetListVerticalTransformation(const float Y);
    float GetListPendingVerticalTransformation(const float Y);
    void OnSongChange(std::shared_ptr<Game::Song> MySong, uint8_t difindex);
    void OnSongSelect(std::shared_ptr<Game::Song> MySong, uint8_t difindex);

    void OnDirectoryChange();
    void OnItemClick(int32_t Index, uint32_t boundIndex, std::string Line, std::shared_ptr<Game::Song> Selected);
    void OnItemHover(int32_t Index, uint32_t boundIndex, std::string Line, std::shared_ptr<Game::Song> Selected);
    void OnItemHoverLeave(int32_t Index, uint32_t boundIndex, std::string Line, std::shared_ptr<Game::Song> Selected);

    void TransformItem(int Item, std::shared_ptr<Game::Song> Song, bool IsSelected, int ListItem);
    void TransformString(int Item, std::shared_ptr<Game::Song> Song, bool IsSelected, int ListItem, std::string text);
public:
    ScreenSelectMusic();
    void LoadResources() override;
    void InitializeResources() override;
    bool Run(double Delta) override;
    void Cleanup() override;
    bool HandleInput(int32_t key, KeyEventType code, bool isMouseInput) override;
    bool HandleScrollInput(double xOff, double yOff) override;
};