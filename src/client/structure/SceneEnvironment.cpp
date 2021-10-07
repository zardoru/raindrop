#include <cstdint>
#include <string>
#include <memory>
#include <filesystem>
#include <map>
#include <functional>

#include <rmath.h>

#include <game/GameConstants.h>
#include "../game/Game.h"
#include "../game/PlayscreenParameters.h"
#include "../game/GameState.h"

#include "Transformation.h"
#include "Rendering.h"
#include "Sprite.h"
#include "Texture.h"

#include "SceneEnvironment.h"
#include "ImageList.h"
#include "Logging.h"
#include "GameWindow.h"

#include "LuaManager.h"
#include <LuaBridge/LuaBridge.h>
#include <TextAndFileUtil.h>

#include <GLFW/glfw3.h>
#include "TruetypeFont.h"
#include "Configuration.h"
#include "BindingsManager.h"

/// All the other scenes have one of these.
// No @{Object2D} should not be created on the global scope. Only on the callbacks given by the SceneBase.
// Any custom screens will only have what is provided by this module.
/// @themescript SceneBase

void CreateLuaInterface(LuaManager *AnimLua);

bool LuaAnimation(LuaManager *Lua, const std::string& Func, Sprite *Target, float Frac) {
    if (Lua->CallFunction(Func.c_str(), 2, 1)) {
        Lua->PushArgument(Frac);
        luabridge::push(Lua->GetState(), Target);

        if (Lua->RunFunction())
            return Lua->GetFunctionResult() > 0;
        else
            return false;
    } else return false;
}

void SceneEnvironment::StopAnimationsForTarget(Sprite *Target) {
    for (auto i = Animations.begin();
         i != Animations.end();
            ) {
        if (i->Target == Target) {
            i = Animations.erase(i);
            if (i == Animations.end()) break;
            else continue;
        }

        i++;
    }
}

void SceneEnvironment::RunIntro(float Fraction, float Delta) {
    if (mFrameSkip) {
        mFrameSkip = false;
        return;
    }

    /// Update the screen's intro state
    // @callback UpdateIntro
    // @param fraction The percentage of the intro that is done.
    // @param delta The time passed since last frame.
    if (Lua->CallFunction("UpdateIntro", 2)) {
        Lua->PushArgument(Fraction);
        Lua->PushArgument(Delta);
        Lua->RunFunction();
    }

    DrawFromLayer(0);
}

void SceneEnvironment::RunExit(float Fraction, float Delta) {
    if (mFrameSkip) {
        mFrameSkip = false;
        return;
    }
    /// Update the screen's transition into the next screen.
    // @callback UpdateExit
    // @param fraction The percentage of the intro that is done.
    // @param delta The time passed since last frame.
    if (Lua->CallFunction("UpdateExit", 2)) {
        Lua->PushArgument(Fraction);
        Lua->PushArgument(Delta);
        Lua->RunFunction();
    }

    DrawFromLayer(0);
}

float SceneEnvironment::GetIntroDuration() {
    /// How long the intro section lasts.
    // @modvar IntroDuration
    return std::max(Lua->GetGlobalD("IntroDuration"), 0.0);
}

float SceneEnvironment::GetExitDuration() {
    /// How long the outro section lasts.
    // @modvar ExitDuration
    return std::max(Lua->GetGlobalD("ExitDuration"), 0.0);
}

void SceneEnvironment::AddLuaAnimation(Sprite *Target, const std::string &FuncName,
                                       int Easing, float Duration, float Delay) {
    Animation Anim;
    Anim.Function = bind(LuaAnimation, Lua.get(), FuncName, Target, std::placeholders::_1);
    Anim.Easing = (Animation::EEaseType) Easing;
    Anim.Duration = Duration;
    Anim.Delay = Delay;
    Anim.Target = Target;

    Animations.push_back(Anim);
}

SceneEnvironment::SceneEnvironment(const char *ScreenName, bool initUI) {
    Animations.reserve(10);
    Lua = std::make_shared<LuaManager>();
    Lua->RegisterStruct("GOMAN", this);


    GameState::GetInstance().InitializeLua(Lua->GetState());

    /// Automatic instance of SceneEnvironment for script use.
    // @autoinstance Engine
    CreateLuaInterface(Lua.get());
    Images = std::make_shared<ImageList>(true);
    mFrameSkip = true;

    mScreenName = ScreenName;
}

TruetypeFont *SceneEnvironment::CreateTTF(const char *Dir) {
    auto *Ret = new TruetypeFont(Dir);
    ManagedFonts.push_back(Ret);
    return Ret;
}

SceneEnvironment::~SceneEnvironment() {
    /// Called when the scene environment will be destroyed.
    // @callback Cleanup
    if (Lua->CallFunction("Cleanup")) {
        Lua->RunFunction();
    }

    // Remove all managed drawable objects.
    for (auto i : ManagedObjects)
        delete i;

    for (auto i : ManagedFonts)
        delete i;

    ManagedObjects.clear();
    ManagedFonts.clear();
}

void SceneEnvironment::Preload(const std::filesystem::path& Filename, std::string array_name) {
    mInitScript = Filename;

    if (!Lua->RunScript(Filename)) {
        Log::LogPrintf("Couldn't run lua script while preloading: %s\n", Lua->GetLastError().c_str());
    }

    if (Lua->UseArray(array_name)) {
        Lua->StartIteration();

        while (Lua->IterateNext()) {
            auto s = GameState::GetInstance().GetSkinFile(Lua->NextGString());
            Images->AddToList(s, "");
            Lua->Pop();
        }

        Lua->Pop();
    }
}

void SceneEnvironment::Sort() {
    std::stable_sort(Objects.begin(), Objects.end(),
                     [](const Drawable2D *A, const Drawable2D *B) -> bool { return A->GetZ() < B->GetZ(); });
}

Sprite *SceneEnvironment::CreateObject() {
    auto Out = new Sprite;
    ManagedObjects.push_back(Out);
    AddTarget(Out, true); // Destroy on reload
    return Out;
}

bool SceneEnvironment::IsManagedObject(Drawable2D *Obj) {
    for (auto i : ManagedObjects) {
        if (Obj == i)
            return true;
    }

    return false;
}

void SceneEnvironment::Initialize(const std::filesystem::path& Filename, bool RunScript) {
    if (!mInitScript.wstring().length() && Filename.wstring().length())
        mInitScript = Filename;

    if (RunScript) {
        if (!Lua->RunScript(mInitScript)) {
            Log::LogPrintf("Couldn't load script %s: %s", mInitScript.string().c_str(), Lua->GetLastError().c_str());
        }
    }

    /// This function is called at the initialization phase of the screen.
    // @callback Init
    if (Lua->CallFunction("Init"))
        Lua->RunFunction();

    Images->LoadAll();
}

void SceneEnvironment::AddTarget(Drawable2D *target, bool IsExternal) {
    Objects.push_back(target);

    if (IsExternal)
        ExternalObjects.push_back(target);

    Sort();
}

void SceneEnvironment::AddSpriteTarget(Sprite *target) {
    AddTarget(target, false);
}

void SceneEnvironment::AddLuaTarget(Sprite *target, std::string Varname) {
    lua_State *L = Lua->GetState();
    luabridge::push(L, target);
    lua_setglobal(L, Varname.c_str());
}

void SceneEnvironment::StopManagingObject(Drawable2D *Obj) {
    for (auto i = ManagedObjects.begin(); i != ManagedObjects.end(); ++i) {
        if (Obj == *i) {
            ManagedObjects.erase(i);
            return;
        }
    }
}

void SceneEnvironment::RemoveManagedObject(Drawable2D *Obj) {
    for (auto i = ManagedObjects.begin(); i != ManagedObjects.end(); ++i) {
        if (*i == Obj) {
            RemoveTarget(*i);
            delete *i;
            ManagedObjects.erase(i);
            return;
        }
    }
}

void SceneEnvironment::HandleScrollInput(double x_off, double y_off) {
    /// Called when the mouse scrolls.
    // @callback ScrollEvent
    // @param xoff Change in X scroll.
    // @param yoff Change in Y scroll.
    if (Lua->CallFunction("ScrollEvent", 2)) {
        Lua->PushArgument(x_off);
        Lua->PushArgument(y_off);
        Lua->RunFunction();
    }
}

void SceneEnvironment::RemoveManagedObjects() {
    for (auto i : ManagedObjects) {
        RemoveTarget(i);
        delete i;
    }

    ManagedObjects.clear();
}

void SceneEnvironment::RemoveExternalObjects() {
    for (auto i : ExternalObjects) {
        RemoveTarget(i);
    }

    ExternalObjects.clear();
}

void SceneEnvironment::RemoveTarget(Drawable2D *target) {
    for (auto i = Objects.begin(); i != Objects.end();) {
        if (*i == target) {
            i = Objects.erase(i);
            continue;
        }

        if (i == Objects.end())
            break;

        ++i;
    }
}

void SceneEnvironment::DrawTargets(double TimeDelta) {
    UpdateTargets(TimeDelta);

    DrawFromLayer(0);
}

void SceneEnvironment::UpdateTargets(double TimeDelta) {
    if (mFrameSkip) {
        mFrameSkip = false;
        return;
    }

    for (auto i = Animations.begin();
         i != Animations.end();) {
        if (i->Delay > 0) {
            i->Delay -= TimeDelta; // Still waiting for this to start.

            if (i->Delay < 0) // We rolled into the negatives.
                i->Time += -i->Delay; // Add it to passed time, to pretend it started right on time.
            else {
                i++;
                continue; // It hasn't began yet, so keep at it.
            }
        } else
            i->Time += TimeDelta;

        if (i->Time >=
            i->Duration) // The animation is done. Call the function one last time with value 1 so it's completed.
        {
            i->Function(1);
            i = Animations.erase(i);
            if (i == Animations.end()) break;
            else continue;
        }

        float frac;

        switch (i->Easing) {
            case Animation::EaseIn:
                frac = pow(i->Time / i->Duration, 2);
                break;
            case Animation::EaseOut:
                frac = i->Time / i->Duration;
                frac = -frac * (frac - 2);
                break;
            case Animation::EaseLinear:
            default:
                frac = i->Time / i->Duration;
        }

        if (!i->Function(frac)) // Says the animation is over?
        {
            i = Animations.erase(i);
            if (i == Animations.end()) break;
            else continue;
        }

        i++;
    }

    /// Main update loop. Called every frame.
    // @callback Update
    // @param delta Change in time since last frame.
    if (Lua->CallFunction("Update", 1)) {
        Lua->PushArgument(TimeDelta);
        Lua->RunFunction();
    }

}

void SceneEnvironment::ReloadUI() {
}

/* This function right now is broken beyond repair. Don't mind it. */
void SceneEnvironment::ReloadScripts() {
    auto InitScript = mInitScript;
    this->~SceneEnvironment();
    new(this) SceneEnvironment(mScreenName.c_str(), false);

    Initialize(InitScript);
}

void SceneEnvironment::ReloadAll() {
    //ReloadUI();
    ReloadScripts();
}

void SceneEnvironment::SetScreenName(std::string sname) {
    mScreenName = sname;
}

void SceneEnvironment::DrawUntilLayer(uint32_t Layer) {
    for (auto i : Objects) {
        if (i == nullptr) { /* throw an error */ continue; }
        if (i->GetZ() <= Layer)
            i->Render();
    }
}

void SceneEnvironment::DrawFromLayer(uint32_t Layer) {
    for (auto &Object : Objects) {
        if (Object->GetZ() >= Layer)
            Object->Render();
    }
}

LuaManager *SceneEnvironment::GetEnv() {
    return Lua.get();
}

bool SceneEnvironment::HandleInput(int32_t key, bool isPressed, bool isMouseInput) {
    /// Called when a key is pressed or released.
    // @callback KeyEvent
    // @param key The key code.
    // @param type The type of event. 1 is press, 2 is release.
    // @param isMouseInput Whether this is a mouse button press.
    if (Lua->CallFunction("KeyEvent", 3)) {
        Lua->PushArgument(key);
        Lua->PushArgument(isPressed);
        Lua->PushArgument(isMouseInput);
        Lua->RunFunction();
    }

    return true;
}

bool SceneEnvironment::HandleTextInput(int codepoint) {
    return false;
}

ImageList *SceneEnvironment::GetImageList() {
    return Images.get();
}

void SceneEnvironment::DoEvent(std::string EventName, int Return) {
    if (Lua->CallFunction(EventName.c_str(), 0, Return))
        Lua->RunFunction();
}

void SceneEnvironment::RemoveSpriteTarget(Sprite *Targ) {
    RemoveTarget(Targ);
}
