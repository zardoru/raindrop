#include <filesystem>
#include <array>

#include <catch.hpp>
#include <LuaManager.h>
#include <game/Gauge.h>
#include <game/gauges/GaugeOsuMania.h>
#include <game/VSRGMechanics.h>
#include <game/ScoreKeeper7K.h>
#include <iostream>

constexpr auto epsilon = 0.001; // one ms
constexpr auto TIME_RANGE = 10000;

using namespace rd;

struct OMSetup {
    std::shared_ptr<ScoreKeeper> sk;
    RaindropMechanics mech;

    // always lane 0
    bool LaneDown;

    OMSetup() : mech(true) {
        sk = std::make_shared<ScoreKeeper>();
        sk->setODWindows(0);
        mech.Setup(nullptr, sk);
        sk->setTotalObjects(100, 0);

        // SetLaneHoldingState is set if we're currently hitting a hold.
        // we don't really need this.
        /*mech.SetLaneHoldingState = [&](uint32_t, bool state) {
            LaneDown = state;
        };*/

        mech.IsLaneKeyDown = [&](uint32_t) {
            return LaneDown;
        };

        mech.HitNotify = [&](double dev, uint32_t lane, bool hold, bool should_break) {
            sk->hitNote(dev, lane, NoteJudgmentPart::NOTE);
        };

        mech.MissNotify = [&](double t, uint32_t, bool hold, bool nobreakcombo, bool earlymiss) {
            sk->missNote(nobreakcombo, earlymiss, true);
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
            REQUIRE(s.sk->hitNote(judgems + 2, 0, NoteJudgmentPart::NOTE) == j);

            j = (ScoreKeeperJudgment)((int) j + 1);
        }
    }

    SECTION("Judgments from W1 to W5 are correct at the limit Early (OD0)") {
        auto j = SKJ_W1;
        for (auto judgems : JudgmentValues) {
            REQUIRE(s.sk->hitNote(judgems, 0, NoteJudgmentPart::NOTE) == j);

            j = (ScoreKeeperJudgment)((int) j + 1);
        }
    }

    SECTION("Judgments from W1 to W5 are correct at the limit Late (OD0)") {
        auto j = SKJ_W1;
        for (auto judgems : JudgmentValuesLate) {
            REQUIRE(s.sk->hitNote(judgems, 0, NoteJudgmentPart::NOTE) == j);

            j = (ScoreKeeperJudgment)((int) j + 1);
        }
    }

    SECTION("What even happened here?") {
        REQUIRE(s.sk->hitNote(-127.73032683987395, 0, NoteJudgmentPart::NOTE) != -1);
    }

    double earlyhitWindow = s.sk->getEarlyHitCutoffMS() / 1000.0;
    double lateCutoff = s.sk->getLateMissCutoffMS() / 1000.0;
    double lnCutoff = s.sk->getJudgmentWindow(SKJ_W3);
    TrackNote t(
            NoteData{0, 10}
    );

    SECTION("Long note tail windows are hit inside the hit window") {
        TrackNote lt = t;

        lt.Hit(); // we need this flag to hit the tail
        REQUIRE(s.mech.OnReleaseLane(lt.GetEndTime(), &lt, 0));

        lt.Reset();
        lt.Hit();
        REQUIRE(s.mech.OnReleaseLane(lt.GetEndTime() - earlyhitWindow + epsilon, &lt, 0));

        lt.Reset();
        lt.Hit();
        REQUIRE(s.mech.OnReleaseLane(lt.GetEndTime() + lateCutoff - epsilon, &lt, 0));
    }

    SECTION("Long note tails have proper lenience when hit") {
        TrackNote lt = t;

// update
        lt.Hit();
        REQUIRE_FALSE(s.mech.OnUpdate(lt.GetEndTime() - lateCutoff + epsilon, &lt, 0));
        REQUIRE_FALSE(s.mech.OnUpdate(lt.GetEndTime() + lateCutoff - epsilon, &lt, 0));
        REQUIRE_FALSE(s.mech.OnUpdate(lt.GetEndTime() + epsilon, &lt, 0));

        lt.Reset();
        lt.Hit();
        REQUIRE(s.mech.OnUpdate(lt.GetEndTime() + lateCutoff + epsilon, &lt, 0));


// release on time
        lt.Reset();
        lt.Hit();
        int misses = s.sk->getJudgmentCount(SKJ_MISS);
        REQUIRE(s.mech.OnReleaseLane(lt.GetEndTime() + lateCutoff - epsilon, &lt, 0));
        REQUIRE(s.sk->getJudgmentCount(SKJ_MISS) == misses);

        lt.Reset();
        lt.Hit();
        REQUIRE(s.mech.OnReleaseLane(lt.GetEndTime() + epsilon, &lt, 0));
        REQUIRE(s.sk->getJudgmentCount(SKJ_MISS) == misses);

        lt.Reset();
        lt.Hit();
        REQUIRE(s.mech.OnReleaseLane(lt.GetEndTime() - lateCutoff + epsilon, &lt, 0));
        REQUIRE(s.sk->getJudgmentCount(SKJ_MISS) == misses);

// too early/late release
        lt.Reset();
        lt.Hit();
        REQUIRE(s.mech.OnReleaseLane(lt.GetEndTime() - earlyhitWindow - epsilon, &lt, 0));
        REQUIRE(s.sk->getJudgmentCount(SKJ_MISS) == misses + 1);

        lt.Reset();
        lt.Hit();
        REQUIRE(s.mech.OnReleaseLane(lt.GetEndTime() + lateCutoff + epsilon, &lt, 0));
        REQUIRE(s.sk->getJudgmentCount(SKJ_MISS) == misses + 2);
    }

    SECTION("Long note tails miss only after the tail end is done when not hit") {
        TrackNote lt = t;
        REQUIRE_FALSE(s.mech.OnUpdate(5, &lt, 0));

        lt.DisableHead(); // otherwise, it'll miss the head
        REQUIRE_FALSE(s.mech.OnUpdate(t.GetEndTime() - epsilon, &lt, 0));
        REQUIRE(s.mech.OnUpdate(t.GetEndTime() + epsilon, &lt, 0));
    }


    SECTION("No runtime errors across a big range of time") {
        for (int i = -TIME_RANGE; i <= TIME_RANGE; i++) {
            double t = (double) i / 1000.0;
            REQUIRE_NOTHROW(s.sk->hitNote(t, 0, NoteJudgmentPart::NOTE));
        }
    }

    SECTION("Hit weaks should break combo.") {
        s.sk->init();
        s.sk->setODWindows(0);
        for (int i = 0; i < 50; i++)
            s.sk->hitNote(0, 0, NoteJudgmentPart::NOTE);

        REQUIRE(s.sk->getScore(ST_COMBO) == 50);
        REQUIRE(s.sk->hitNote(-s.sk->getEarlyHitCutoffMS() + 1, 0, NoteJudgmentPart::NOTE) == SKJ_MISS);
        REQUIRE(s.sk->getScore(ST_COMBO) == 0);

        /* late version */
        s.sk->init();
        s.sk->setODWindows(0);
        for (int i = 0; i < 50; i++)
            s.sk->hitNote(0, 0, NoteJudgmentPart::NOTE);

        REQUIRE(s.sk->getScore(ST_COMBO) == 50);
        REQUIRE(s.sk->hitNote(s.sk->getEarlyHitCutoffMS() - 1, 0, NoteJudgmentPart::NOTE) == SKJ_MISS);
        REQUIRE(s.sk->getScore(ST_COMBO) == 0);
    }

    SECTION("[Mechanics] Early Weak hits should break combo.") {
        s.sk->init();
        s.sk->setODWindows(0);
        for (int i = 0; i < 50; i++)
            s.sk->hitNote(0, 0, NoteJudgmentPart::NOTE);

        TrackNote t(NoteData{ 0, 0 });
        REQUIRE(s.sk->getScore(ST_COMBO) == 50);
        REQUIRE(s.mech.OnPressLane((-s.sk->getEarlyHitCutoffMS() + 1) / 1000.0, &t, 0) == true);
        REQUIRE(s.sk->getScore(ST_COMBO) == 0);

        /* late version */
        s.sk->init();
        s.sk->setODWindows(0);
        for (int i = 0; i < 50; i++)
            s.sk->hitNote(0, 0, NoteJudgmentPart::NOTE);


        t.Reset();
        REQUIRE(s.sk->getScore(ST_COMBO) == 50);
        REQUIRE(s.mech.OnPressLane((s.sk->getLateMissCutoffMS() - 1) / 1000.0, &t, 0) == true);
        REQUIRE(s.sk->getScore(ST_COMBO) == 0);
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
            gauge.Setup(0, 0, od);
            od++;

            auto mtf = 0;
            while (gauge.GetGaugeValue() > 0) {
                gauge.Update(SKJ_MISS, false, 0);
                mtf ++;
            }

            INFO("OD " << od - 1 << " Misses To Fail = " << mtf);

            auto test_htf = 0;
            while (gauge.GetGaugeValue() < 1) {
                gauge.Update(SKJ_W1, false, 0);
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
            gauge.Setup(0, 0, od);

            for (int i = 0; i < target - 1; i++) {
                gauge.Update(SKJ_MISS, false, 0);
                INFO(od << " - " << i << "/" << target);
                REQUIRE(gauge.GetGaugeValue() > 0);
            }

            gauge.Update(SKJ_MISS, false, 0);
            REQUIRE(gauge.GetGaugeValue() == 0);
            od += 1;
        }
    }
}