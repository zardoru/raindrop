#pragma once

class Application
{
    double oldTime;
    std::shared_ptr<Screen> Game;

    enum
    {
        MODE_NULL,
        MODE_PLAY,
        MODE_CONVERT,
        MODE_GENSONGCACHE,
        MODE_VSRGPREVIEW,
        MODE_STOPPREVIEW,
        MODE_CUSTOMSCREEN,
		MODE_GENFONTCACHE,
		MODE_TEST
    }RunMode;

    void ParseArgs(int, char **);

    std::filesystem::path InFile, OutFile, InFontTextFile;

    // VSRG-Specific
    enum class CONVERTMODE
    {
        CONV_BMS,
        CONV_UQBMS,
        CONV_SM,
        CONV_OM,
        CONV_NPS,
		CONV_ACCTEST
    } ConvertMode;

    int Measure;
    int difIndex;
    std::string Author;

    bool Upscroll;

    void SetupPreviewMode();
    bool PollIPC();

public:

    Application(int argc, char *argv[]);

    void HandleInput(int32_t key, KeyEventType state, bool isMouseInput);
    void HandleScrollInput(double xOff, double yOff);

    void Init();
    void Run();
    void Close();
    void HandleTextInput(unsigned cp);
};