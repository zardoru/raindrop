#include <rmath.h>
#include <cstdint>
#include <filesystem>

#include <client/backend/Transformation.h>
#include <client/backend/Rendering.h>
#include <client/backend/Sprite.h>
#include <client/backend/LuaManager.h>
#include <client/game/Noteskin.h>
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
}

TEST_CASE("Noteskin can be used with no context", "[noteskin]")
{
    EnsureConfigurationLoaded();
    Noteskin n(nullptr);

    SECTION("Noteskin doesn't crash with no context") {

        REQUIRE_NOTHROW(n.SetupNoteskin(false, 4));
        REQUIRE_NOTHROW(n.Validate());
        REQUIRE_NOTHROW(n.DrawHoldBody(0, 0, 0, 0));
    }
}
