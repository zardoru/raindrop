#include <filesystem>
#include <rmath.h>

#include "Transformation.h"
#include "Rendering.h"
#include "Sprite.h"

#include "LuaManager.h"
#include <LuaBridge/LuaBridge.h>

#include "../game/Noteskin.h"

void Noteskin::add_script_classes() {
    luabridge::getGlobalNamespace(noteskin_lua_.get_lua_state())
            /// @engineclass NoteskinObject
            .beginClass<Noteskin>("Noteskin")
                    /***
                     Draw an Object2D instance.
                     @function Render
                     @tparam obj Object2D The object to draw.
                    */
            .addFunction("Render", &Noteskin::lua_render)
                    /// Vertical offset of the barline.
                    // @property BarlineOffset
            .addData("BarlineOffset", &Noteskin::barline_offset_)
                    /// Start X position of the barline.
                    // @property BarlineStartX
            .addData("BarlineStartX", &Noteskin::barline_start_x_)
                    /// Width of the barline.
                    /// @property BarlineWidth
            .addData("BarlineWidth", &Noteskin::barline_width_)
                    /// Whether the barline is displayed or not.
                    // @property BarlineEnabled
            .addData("BarlineEnabled", &Noteskin::barline_enabled_)
                    /// If enabled, holds will resize when they're being held.
                    // @property DecreaseHoldSizeWhenBeingHit
            .addData("DecreaseHoldSizeWhenBeingHit", &Noteskin::decrease_hold_size_when_being_hit_)
                    /// If enabled, notes will linger on the judgment line a la LR2.
                    // @property DanglingHeads
            .addData("DanglingHeads", &Noteskin::dangling_heads_)
                    /// The vertical size of a note. Used for culling purposes.
                    // @property NoteScreenSize
            .addData("NoteScreenSize", &Noteskin::note_screen_size_)
                    /// Distance of the judgment from the top when upscrolling, distance from the bottom when downscrolling.
                    // @property JudgmentY
            .addData("JudgmentY", &Noteskin::judgment_y_)
                    /// Read-only property informing of the amount of channels currently active.
                    // @roproperty Channels
            .addProperty("Channels", &Noteskin::get_channels)
            .endClass();
}
