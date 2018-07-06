#include "pch.h"

#include "Screen.h"
#include "ScreenGameplay7K.h"
#include "LuaBridge.h"

namespace Game
{
namespace VSRG
{
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
    }
} // namespace VSRG
} // namespace Game