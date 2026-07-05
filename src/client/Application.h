#pragma once

class Screen;

class Application
{
    double oldTime;
    std::shared_ptr<Screen> root;

    enum
    {
        MODE_NULL,
        MODE_PLAY,
        MODE_GENSONGCACHE,
        MODE_VSRGPREVIEW,
        MODE_STOPPREVIEW,
        MODE_CUSTOMSCREEN,
		MODE_GENFONTCACHE
    }mode;

    void parse_args(int, char **);

    std::filesystem::path InFile, InFontTextFile;

    int measure_;
    int difIndex;
    std::string Author;

    bool Upscroll;

    void setup_preview_mode();
    bool poll_ipc();

public:

    Application(int argc, char *argv[]);

    void on_input(int32_t key, bool is_pressed, bool is_mouse_input) const;
    void on_scroll_input(double x_off, double y_off) const;

    void init();
    void run();
    void close();
    void on_text_input(unsigned cp) const;
};