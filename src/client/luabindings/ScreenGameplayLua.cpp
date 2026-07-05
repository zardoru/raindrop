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


#include <sndio/Audiofile.h>
#include <sndio/AudioSourceOJM.h>

#include "LuaManager.h"
#include <LuaBridge/LuaBridge.h>

#include <game/VSRGMechanics.h>
#include "../game/PlayscreenParameters.h"
#include "../game/PlayerContext.h"


#include "Transformation.h"
#include "Rendering.h"
#include "Sprite.h"
#include "../bga/BackgroundAnimation.h"

#include "../structure/Screen.h"
#include "../screens/ScreenGameplay.h"


void ScreenGameplay::add_script_classes(LuaManager* Env)
{
#define f(n, x) addProperty(n, &ScreenGameplay::x)
    luabridge::getGlobalNamespace(Env->get_lua_state())
        /// @engineclass ScreenGameplay7K
        .beginClass<ScreenGameplay>("ScreenGameplay7K")
        // Whether the song time is advancing.
        /// @roproperty Active
        .f("Active", is_active)
        /// Get player playing on this screen.
        // @function GetPlayer
        // @param id Index of the player to return.
        // @return nil if not found, Player if found.
        .addFunction("GetPlayer", &ScreenGameplay::GetPlayerContext)
        .addFunction("SetPlayerClip", &ScreenGameplay::set_player_clip)
        .addFunction("DisablePlayerClip", &ScreenGameplay::disable_player_clip)
        // All of these depend on the player.
        .endClass();

    luabridge::push(Env->get_lua_state(), this);
    lua_setglobal(Env->get_lua_state(), "Game");
}
