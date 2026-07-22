#include <rmath.h>
#include <cstdint>
#include <filesystem>
#include <string>
#include <utility>

#include <client/backend/Transformation.h>
#include <client/backend/Rendering.h>
#include <client/backend/Sprite.h>
#include <client/backend/LuaManager.h>
#include <client/game/Noteskin.h>
#include <client/game/GameState.h>
#include <client/structure/Configuration.h>

#include <catch2/catch_test_macros.hpp>

namespace {
    void EnsureConfigurationLoaded()
    {
        static const bool initialized = [] {
            Configuration::SetConfigFile("tests/files/test_config.ini");
            Configuration::Initialize();
            return true;
        }();

        (void)initialized;
    }

    class GameStateSkinScope {
        GameState& game_state_;
        std::string old_system_folder_;
        std::string old_skin_;

    public:
        GameStateSkinScope(std::string system_folder, std::string skin)
            : game_state_(GameState::get_instance()),
              old_system_folder_(game_state_.filesystem().get_directory_prefix()),
              old_skin_(game_state_.get_skin())
        {
            game_state_.set_system_folder(std::move(system_folder));
            game_state_.set_skin(std::move(skin));
        }

        ~GameStateSkinScope()
        {
            game_state_.set_system_folder(old_system_folder_);
            game_state_.set_skin(old_skin_);
        }
    };

    void CheckReturnedCallbackDispatch(Noteskin& noteskin)
    {
        REQUIRE_NOTHROW(noteskin.finalize_loading());
        CHECK(noteskin.get_barline_width() == 123);

        REQUIRE_NOTHROW(noteskin.update(2, 3));
        CHECK(noteskin.get_barline_start_x() == 23);

        REQUIRE_NOTHROW(noteskin.draw_hold_body(1, 2, 3, 4));
        CHECK(noteskin.get_note_offset() == 10);
    }
}

TEST_CASE("Noteskin can be used with no context", "[noteskin]")
{
    EnsureConfigurationLoaded();
    Noteskin n(nullptr);

    SECTION("Noteskin doesn't crash with no context") {

        REQUIRE_NOTHROW(n.init_noteskin(false, 4));
        REQUIRE_NOTHROW(n.finalize_loading());
        REQUIRE_NOTHROW(n.draw_hold_body(0, 0, 0, 0));
    }
}

TEST_CASE("Noteskin call_callback uses returned callbacks before globals", "[noteskin]")
{
    EnsureConfigurationLoaded();

    GameStateSkinScope skin_scope("tests/files/", "returned_callbacks");

    Noteskin n(nullptr);
    REQUIRE_NOTHROW(n.init_noteskin(false, 4));
    CheckReturnedCallbackDispatch(n);
}
