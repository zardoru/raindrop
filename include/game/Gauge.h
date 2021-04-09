#pragma once

#include <game/GameConstants.h>

namespace rd {
    class GaugeParameters {};

    class Gauge { /* all gauges and values are on a 0-1 scale for 0 to 100% */
    protected:
        double lifebar_amount = 1;
    public:
        virtual void DefaultSetup();
        virtual void Setup(double total, double max_notes, double strictness);
        virtual void Reset() = 0; /* Note: Setup must have been called */

        /* is_early means whether this is not a late miss (late miss = note was simply not pressed) */
        virtual void Update(ScoreKeeperJudgment skj, bool is_early, float mine_value = -0.16) = 0;
        virtual bool HasFailed(bool song_ended);

        virtual bool HasDelayedFailure();
        virtual double GetGaugeValue(); /* from 0 to 1 */
        virtual double GetGaugeDisplayValue(); /* also from 0 to 1, but quantized and whatever. */
    };
}
