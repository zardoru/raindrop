file = {
    "GameData/Scripts", 
    "src/NoteskinLua.cpp",
    "src/Noteskin.cpp",
    "src/PlayerContext.cpp",
    "src/ScreenGameplay7K.cpp",
    "src/ScreenGameplay7KLua.cpp",
    "src/ScreenLoading.cpp",
    "src/GameStateLua.cpp",
    "src/ScreenMainMenu.cpp",
    "src/Object2DLua.cpp",
    "src/ShaderLua.cpp",
    "src/SceneEnvironmentLua.cpp",
    "src/StringsLua.cpp",
    "src/TransformationLua.cpp",
    "src/UtilityLua.cpp",
    "src/ScoreKeeper7K_Lua.cpp",
    "src/SceneEnvironment.cpp"
}
project = "raindrop"
title = "raindrop API Reference"
sort = true
merge = true
new_type("callback","Engine Callbacks", false)
new_type("modvar","Module Variables", false)
new_type("property","Properties")
new_type("enumproperty","Enum Properties", false, "value")
new_type("roproperty","Read only Properties")
new_type("autoinstance","Built-in Objects")
new_type("themescript", "Hardcoded theme scripts", true)
new_type("engineclass", "Engine Classes", true)
new_type("gamemodule", "Lua game modules", true)
new_type("enum", "Enums", false, "symbols")
add_language_extension('cpp', 'c')