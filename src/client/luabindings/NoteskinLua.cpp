#include <game/Song.h>
#include <rmath.h>

#include "Transformation.h"
#include "Rendering.h"
#include "Sprite.h"

#include "../LuaManager.h"
#include <LuaBridge/LuaBridge.h>

#include "../Noteskin.h"

void Noteskin::AddScriptClasses() {
    luabridge::getGlobalNamespace(NoteskinLua.GetState())
            /// @engineclass NoteskinObject
            .beginClass<Noteskin>("Noteskin")
                    /***
                     Draw an Object2D instance.
                     @function Render
                     @tparam obj Object2D The object to draw.
                    */
            .addFunction("Render", &Noteskin::LuaRender)
                    /// Vertical offset of the barline.
                    // @property BarlineOffset
            .addData("BarlineOffset", &Noteskin::BarlineOffset)
                    /// Start X position of the barline.
                    // @property BarlineStartX
            .addData("BarlineStartX", &Noteskin::BarlineStartX)
                    /// Width of the barline.
                    /// @property BarlineWidth
            .addData("BarlineWidth", &Noteskin::BarlineWidth)
                    /// Whether the barline is displayed or not.
                    // @property BarlineEnabled
            .addData("BarlineEnabled", &Noteskin::BarlineEnabled)
                    /// If enabled, holds will resize when they're being held.
                    // @property DecreaseHoldSizeWhenBeingHit
            .addData("DecreaseHoldSizeWhenBeingHit", &Noteskin::DecreaseHoldSizeWhenBeingHit)
                    /// If enabled, notes will linger on the judgment line a la LR2.
                    // @property DanglingHeads
            .addData("DanglingHeads", &Noteskin::DanglingHeads)
                    /// The vertical size of a note. Used for culling purposes.
                    // @property NoteScreenSize
            .addData("NoteScreenSize", &Noteskin::NoteScreenSize)
                    /// Distance of the judgment from the top when upscrolling, distance from the bottom when downscrolling.
                    // @property JudgmentY
            .addData("JudgmentY", &Noteskin::JudgmentY)
                    /// Read-only property informing of the amount of channels currently active.
                    // @roproperty Channels
            .addProperty("Channels", &Noteskin::GetChannels)
            .endClass();
}
