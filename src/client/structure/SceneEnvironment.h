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
    SceneEnvironment(const char* ScreenName, bool initGUI = false);
    ~SceneEnvironment();

    void RemoveManagedObjects();
    void RemoveExternalObjects();

    void ReloadScripts();
    void ReloadUI();
    void ReloadAll();

	void SetScreenName(std::string sname);

    void Preload(const std::filesystem::path& Filename, std::string array_name);
    void Initialize(const std::filesystem::path& Filename = "", bool RunScript = true);
    LuaManager *GetEnv();
    ImageList* GetImageList();

    Sprite* CreateObject();

    void DoEvent(std::string EventName, int Return = 0);
    void AddLuaAnimation(Sprite* Target, const std::string &FName, int Easing, float Duration, float Delay);
    void StopAnimationsForTarget(Sprite* Target);
    void AddTarget(Drawable2D* Targ, bool IsExternal = false);
    void AddSpriteTarget(Sprite* Targ);
    void AddLuaTarget(Sprite *Targ, std::string Varname);
    void AddLuaTargetArray(Sprite *Targ, std::string Varname, std::string Arrname);
    void RemoveTarget(Drawable2D *Targ);
    void DrawTargets(double TimeDelta);

    TruetypeFont* CreateTTF(const char* Dir);

    void Sort();

    void UpdateTargets(double TimeDelta);
    void DrawUntilLayer(uint32_t Layer);
    void DrawFromLayer(uint32_t Layer);

    void RunIntro(float Fraction, float Delta);
    void RunExit(float Fraction, float Delta);

    float GetIntroDuration();
    float GetExitDuration();

    bool HandleInput(int32_t key, bool isPressed, bool isMouseInput);
    bool HandleTextInput(int codepoint);
    bool IsManagedObject(Drawable2D *Obj);
    void StopManagingObject(Drawable2D *Obj);
    void RemoveManagedObject(Drawable2D *Obj);
    void HandleScrollInput(double x_off, double y_off);
};

void DefineSpriteInterface(LuaManager* anim_lua);

void AddRDLuaGlobal(LuaManager * anim_lua);
