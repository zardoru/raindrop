#pragma once

class Drawable2D;
class Sprite;
class LuaManager;
class ImageList;
class TruetypeFont;

struct Animation
{
    std::function <bool(float Fraction)> Function;

    float Time, Duration, Delay;
    enum EEaseType
    {
        EaseLinear,
        EaseIn,
        EaseOut
    } Easing;

    Sprite* Target;

    Animation()
    {
        Time = Delay = 0;
        Duration = std::numeric_limits<float>::infinity();
        Target = nullptr;
    }
};

class SceneEnvironment
{
    std::shared_ptr<LuaManager> Lua;
    std::shared_ptr<ImageList> Images;
    std::vector<Drawable2D*> Objects;
    std::vector<Drawable2D*> ManagedObjects;
    std::vector<Drawable2D*> ExternalObjects;
    std::vector<TruetypeFont*> ManagedFonts;
    std::vector <Animation> Animations;
    bool mFrameSkip;
    std::string mScreenName;
    std::filesystem::path mInitScript;

public:
    SceneEnvironment(const char* screen_name, bool initGUI = false);
    ~SceneEnvironment();

    void remove_managed_objects();
    void remove_external_objects();

    void reload_scripts();
    void ReloadUI();
    void reload_all();

	void set_screen_name(const std::string &sname);

    void preload(const std::filesystem::path& Filename, std::string array_name);
    void initialize(const std::filesystem::path& filename = "", bool run_script = true);
    LuaManager *get_script_manager() const;
    ImageList* get_image_list() const;

    Sprite* create_object();

    void trigger_event(const std::string &event_name, int Return = 0) const;
    void add_lua_animation(Sprite* target, const std::string &FName, int easing, float duration, float delay);
    void StopAnimationsForTarget(Sprite* Target);
    void add_target(Drawable2D* target, bool is_external = false);
    void add_sprite_target(Sprite* Targ);
    void remove_sprite_target(Sprite* Targ);
    void add_lua_target(Sprite *Targ, std::string Varname) const;
    void AddLuaTargetArray(Sprite *Targ, std::string Varname, std::string Arrname);
    void remove_target(Drawable2D *Targ);
    void draw_targets(double TimeDelta);

    TruetypeFont* create_ttf(const char* Dir);

    void sort();

    void update_targets(double TimeDelta);
    void draw_until_layer(uint32_t layer) const;
    void draw_from_layer(uint32_t layer) const;

    void RunIntro(float Fraction, float Delta);
    void RunExit(float Fraction, float Delta);

    float get_intro_duration() const;
    float get_exit_duration() const;

    bool on_input(int32_t key, bool is_pressed, bool is_mouse_input) const;

    static bool handle_text_input(int codepoint);
    bool is_managed_object(Drawable2D *Obj) const;
    void stop_managing_object(Drawable2D *Obj);
    void remove_managed_object(Drawable2D *Obj);
    void on_scroll_input(double x_off, double y_off) const;
};

void DefineSpriteInterface(LuaManager* anim_lua);

void AddRDLuaGlobal(LuaManager * anim_lua);
