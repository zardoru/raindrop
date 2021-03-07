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


#include <Audiofile.h>
#include <AudioSourceOJM.h>

#include "LuaManager.h"
#include <LuaBridge/LuaBridge.h>

#include <game/Song.h>
#include <game/VSRGMechanics.h>
#include "../game/PlayscreenParameters.h"
#include "../game/PlayerContext.h"


#include "Transformation.h"
#include "Rendering.h"
#include "Sprite.h"
#include "../bga/BackgroundAnimation.h"

#include "../structure/Screen.h"
#include "../screens/ScreenGameplay7K.h"


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
