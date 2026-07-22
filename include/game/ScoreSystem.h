
#pragma once

namespace rd {
    class ScoringSystem {
    public:
        virtual void reset() = 0;

        /* all scoring systems need to be able to respond to all judgments! */
        virtual void update(ScoreKeeperJudgment skj, bool use_w0) = 0;
        virtual long long get_current_score(long long max_notes, bool use_w0) const = 0;
        virtual long long get_max_score(long long max_notes, bool use_w0) = 0;
    };
}