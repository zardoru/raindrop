#pragma once

#include <game/GameConstants.h>

namespace rd {
    class Gauge { /* all gauges and values are on a 0-1 scale for 0 to 100% */
    protected:
        double lifebar_amount_ = 1;
    public:
        virtual void default_setup();
        virtual void setup(double total, long long max_notes, double strictness);
        virtual void reset() = 0; /* Note: setup must have been called */

        /* is_early means whether this is not a late miss (late miss = note was simply not pressed) */
        virtual void update(ScoreKeeperJudgment skj, bool is_early, float mine_value = -0.16) = 0;
        virtual bool has_failed(bool song_ended);

        virtual bool has_delayed_failure();
        virtual double get_gauge_value(); /* from 0 to 1 */
        virtual double get_gauge_display_value(); /* also from 0 to 1, but quantized and whatever. */
    };
}
