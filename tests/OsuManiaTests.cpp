#include <filesystem>
#include <array>

#include <catch2/catch_test_macros.hpp>
#include <LuaManager.h>
#include <game/Gauge.h>
#include <game/gauges/GaugeOsuMania.h>
#include <game/VSRGMechanics.h>
#include <game/ScoreKeeper.h>
#include <iostream>

constexpr auto epsilon = 0.001; // one ms
constexpr auto TIME_RANGE = 10000;

using namespace rd;

namespace {
    struct TestRuntimeNote {
        otoworm::TrackNote source;
        RuntimeNoteStorage storage;

        explicit TestRuntimeNote(const otoworm::NoteData& data) : source(data) {
            storage.push_note(source);
        }

        RuntimeNote* get() {
            return storage.note_at(0);
        }
    };
}

struct OMSetup {
    std::shared_ptr<ScoreKeeper> sk;
    RaindropMechanics mech;

    // always lane 0
    bool LaneDown;

    OMSetup() : mech(true) {
        sk = std::make_shared<ScoreKeeper>();
        sk->set_od_windows(0);
        mech.configure(nullptr, sk);
        sk->set_total_objects(100, 0);

        // SetLaneHoldingState is set if we're currently hitting a hold.
        // we don't really need this.
        /*mech.SetLaneHoldingState = [&](uint32_t, bool state) {
            LaneDown = state;
        };*/

        mech.is_lane_key_down = [&](uint32_t) {
            return LaneDown;
        };

        mech.notify_hit = [&](double dev, uint32_t lane, NoteJudgmentPart part) {
            sk->hit_note(dev, lane, part);
        };

        mech.notify_miss = [&](double t, uint32_t, bool hold, bool nobreakcombo, bool earlymiss) {
            sk->miss_note(nobreakcombo, earlymiss, true);
        };
    }
};

TEST_CASE("osu!mania judgments", "[omjudge]") {
    OMSetup s;

    double JudgmentValues[] = {-16, -64, -97, -127, -151, -188};

    double JudgmentValuesLate[] = {15, 63, 96, 126, 150};

    SECTION("Judgments from W1 to W5 are correct Early (OD0)") {
        auto j = SKJ_W1;
        for (auto judgems : JudgmentValues) {
            REQUIRE(s.sk->hit_note(judgems + 2, 0, NoteJudgmentPart::NOTE) == j);

            j = (ScoreKeeperJudgment)((int) j + 1);
        }
    }

    SECTION("Judgments from W1 to W5 are correct at the limit Early (OD0)") {
        auto j = SKJ_W1;
        for (auto judgems : JudgmentValues) {
            REQUIRE(s.sk->hit_note(judgems, 0, NoteJudgmentPart::NOTE) == j);

            j = (ScoreKeeperJudgment)((int) j + 1);
        }
    }

    SECTION("Judgments from W1 to W5 are correct at the limit Late (OD0)") {
        auto j = SKJ_W1;
        for (auto judgems : JudgmentValuesLate) {
            REQUIRE(s.sk->hit_note(judgems, 0, NoteJudgmentPart::NOTE) == j);

            j = (ScoreKeeperJudgment)((int) j + 1);
        }
    }

    SECTION("What even happened here?") {
        REQUIRE(s.sk->hit_note(-127.73032683987395, 0, NoteJudgmentPart::NOTE) != -1);
    }

    double earlyhitWindow = s.sk->get_early_hit_cutoff_ms() / 1000.0;
    double lateCutoff = s.sk->get_late_miss_cutoff_ms() / 1000.0;
    double lnCutoff = s.sk->get_judgment_window(SKJ_W3);
    const otoworm::NoteData noteData{0, 10};

    TestRuntimeNote t(
            noteData
    );

    SECTION("Long note tail windows are hit inside the hit window") {
        TestRuntimeNote lt(noteData);

        lt.get()->hit(); // we need this flag to hit the tail
        REQUIRE(s.mech.on_release_lane(lt.get()->get_end_time(), lt.get(), 0));

        lt.get()->reset();
        lt.get()->hit();
        REQUIRE(s.mech.on_release_lane(lt.get()->get_end_time() - earlyhitWindow + epsilon, lt.get(), 0));

        lt.get()->reset();
        lt.get()->hit();
        REQUIRE(s.mech.on_release_lane(lt.get()->get_end_time() + lateCutoff - epsilon, lt.get(), 0));
    }

    SECTION("Long note tails have proper lenience when hit") {
        TestRuntimeNote lt(noteData);

// update
        lt.get()->hit();
        REQUIRE_FALSE(s.mech.on_update(lt.get()->get_end_time() - lateCutoff + epsilon, lt.get(), 0));
        REQUIRE_FALSE(s.mech.on_update(lt.get()->get_end_time() + lateCutoff - epsilon, lt.get(), 0));
        REQUIRE_FALSE(s.mech.on_update(lt.get()->get_end_time() + epsilon, lt.get(), 0));

        lt.get()->reset();
        lt.get()->hit();
        REQUIRE(s.mech.on_update(lt.get()->get_end_time() + lateCutoff + epsilon, lt.get(), 0));


// release on time
        lt.get()->reset();
        lt.get()->hit();
        int misses = s.sk->get_judgment_count(SKJ_MISS);
        REQUIRE(s.mech.on_release_lane(lt.get()->get_end_time() + lateCutoff - epsilon, lt.get(), 0));
        REQUIRE(s.sk->get_judgment_count(SKJ_MISS) == misses);

        lt.get()->reset();
        lt.get()->hit();
        REQUIRE(s.mech.on_release_lane(lt.get()->get_end_time() + epsilon, lt.get(), 0));
        REQUIRE(s.sk->get_judgment_count(SKJ_MISS) == misses);

        lt.get()->reset();
        lt.get()->hit();
        REQUIRE(s.mech.on_release_lane(lt.get()->get_end_time() - lateCutoff + epsilon, lt.get(), 0));
        REQUIRE(s.sk->get_judgment_count(SKJ_MISS) == misses);

// too early/late release
        lt.get()->reset();
        lt.get()->hit();
        REQUIRE(s.mech.on_release_lane(lt.get()->get_end_time() - earlyhitWindow - epsilon, lt.get(), 0));
        REQUIRE(s.sk->get_judgment_count(SKJ_MISS) == misses + 1);

        lt.get()->reset();
        lt.get()->hit();
        REQUIRE(s.mech.on_release_lane(lt.get()->get_end_time() + lateCutoff + epsilon, lt.get(), 0));
        REQUIRE(s.sk->get_judgment_count(SKJ_MISS) == misses + 2);
    }

    SECTION("Long note tails miss only after the tail end is done when not hit") {
        TestRuntimeNote lt(noteData);
        REQUIRE_FALSE(s.mech.on_update(5, lt.get(), 0));

        lt.get()->disable_head(); // otherwise, it'll miss the head
        REQUIRE_FALSE(s.mech.on_update(t.get()->get_end_time() - epsilon, lt.get(), 0));
        REQUIRE(s.mech.on_update(t.get()->get_end_time() + epsilon, lt.get(), 0));
    }


    SECTION("No runtime errors across a big range of time") {
        for (int i = -TIME_RANGE; i <= TIME_RANGE; i++) {
            double t = (double) i / 1000.0;
            REQUIRE_NOTHROW(s.sk->hit_note(t, 0, NoteJudgmentPart::NOTE));
        }
    }

    SECTION("Hit weaks should break combo.") {
        s.sk->init();
        s.sk->set_od_windows(0);
        for (int i = 0; i < 50; i++)
            s.sk->hit_note(0, 0, NoteJudgmentPart::NOTE);

        REQUIRE(s.sk->get_score(ST_COMBO) == 50);
        REQUIRE(s.sk->hit_note(-s.sk->get_early_hit_cutoff_ms() + 1, 0, NoteJudgmentPart::NOTE) == SKJ_MISS);
        REQUIRE(s.sk->get_score(ST_COMBO) == 0);

        /* late version */
        s.sk->init();
        s.sk->set_od_windows(0);
        for (int i = 0; i < 50; i++)
            s.sk->hit_note(0, 0, NoteJudgmentPart::NOTE);

        REQUIRE(s.sk->get_score(ST_COMBO) == 50);
        REQUIRE(s.sk->hit_note(s.sk->get_early_hit_cutoff_ms() - 1, 0, NoteJudgmentPart::NOTE) == SKJ_MISS);
        REQUIRE(s.sk->get_score(ST_COMBO) == 0);
    }

    SECTION("[Mechanics] Early Weak hits should break combo.") {
        s.sk->init();
        s.sk->set_od_windows(0);
        for (int i = 0; i < 50; i++)
            s.sk->hit_note(0, 0, NoteJudgmentPart::NOTE);

        TestRuntimeNote t(otoworm::NoteData{ 0, 0 });
        REQUIRE(s.sk->get_score(ST_COMBO) == 50);
        REQUIRE(s.mech.on_press_lane((-s.sk->get_early_hit_cutoff_ms() + 1) / 1000.0, t.get(), 0) == true);
        REQUIRE(s.sk->get_score(ST_COMBO) == 0);

        /* late version */
        s.sk->init();
        s.sk->set_od_windows(0);
        for (int i = 0; i < 50; i++)
            s.sk->hit_note(0, 0, NoteJudgmentPart::NOTE);


        t.get()->reset();
        REQUIRE(s.sk->get_score(ST_COMBO) == 50);
        REQUIRE_FALSE(s.mech.on_press_lane((s.sk->get_early_hit_cutoff_ms() - 1) / 1000.0, t.get(), 0));
        REQUIRE(s.sk->get_score(ST_COMBO) == 50);
    }

    SECTION("Hits to fill match tested data.") {
        static constexpr std::array<int, 11> htf = {
                25,
                28,
                32,
                37,
                43,
                50,
                63,
                83,
                125,
                248,
                1801
        };

        auto od = 0;
        for (const auto &target: htf) {
            GaugeOsuMania gauge;
            gauge.setup(0, 0, od);
            od++;

            auto mtf = 0;
            while (gauge.get_gauge_value() > 0) {
                gauge.update(SKJ_MISS, false, 0);
                mtf ++;
            }

            INFO("OD " << od - 1 << " Misses To Fail = " << mtf);

            auto test_htf = 0;
            while (gauge.get_gauge_value() < 1) {
                gauge.update(SKJ_W1, false, 0);
                test_htf++;
            }

            REQUIRE(test_htf == target);
        }
    }

    SECTION("Misses to fail match tested data.") {
        static constexpr std::array<int, 11> mtf = {
                134,
                67,
                45,
                34,
                27,
                23,
                20,
                17,
                15,
                14,
                13
        };

        auto od = 0;
        for (const auto &target: mtf) {
            GaugeOsuMania gauge;
            gauge.setup(0, 0, od);

            for (int i = 0; i < target - 1; i++) {
                gauge.update(SKJ_MISS, false, 0);
                INFO(od << " - " << i << "/" << target);
                REQUIRE(gauge.get_gauge_value() > 0);
            }

            gauge.update(SKJ_MISS, false, 0);
            REQUIRE(gauge.get_gauge_value() == 0);
            od += 1;
        }
    }
}
