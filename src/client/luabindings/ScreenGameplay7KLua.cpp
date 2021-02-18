#include <exception>
#include <atomic>
#include <memory>
#include <string>
#include <map>
#include <vector>
#include <filesystem>
#include <future>
#include <rmath.h>
#include <queue>
#include <json.hpp>

#include <Audiofile.h>
#include <AudioSourceOJM.h>

#include "../LuaManager.h"
#include <LuaBridge/LuaBridge.h>

#include <game/Song.h>
#include <game/VSRGMechanics.h>
#include "../PlayscreenParameters.h"
#include "../PlayerContext.h"


#include "Transformation.h"
#include "Rendering.h"
#include "Sprite.h"
#include "../BackgroundAnimation.h"

#include "../Screen.h"
#include "../ScreenGameplay7K.h"


void ScreenGameplay::AddScriptClasses(LuaManager* Env)
{
#define f(n, x) addProperty(n, &ScreenGameplay::x)
    luabridge::getGlobalNamespace(Env->GetState())
        /// @engineclass ScreenGameplay7K
        .beginClass<ScreenGameplay>("ScreenGameplay7K")
        // Whether the song time is advancing.
        /// @roproperty Active
        .f("Active", IsActive)
        // Current active song.
        /// @roproperty Song
        .f("Song", GetSong)
        /// Get player playing on this screen.
        // @function GetPlayer
        // @param id Index of the player to return.
        // @return nil if not found, Player if found.
        .addFunction("GetPlayer", &ScreenGameplay::GetPlayerContext)
        .addFunction("SetPlayerClip", &ScreenGameplay::SetPlayerClip)
        .addFunction("DisablePlayerClip", &ScreenGameplay::DisablePlayerClip)
        // All of these depend on the player.
        .endClass();

    luabridge::push(Env->GetState(), this);
    lua_setglobal(Env->GetState(), "Game");
}
