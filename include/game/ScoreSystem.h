
#pragma once

namespace rd {
    class ScoringSystem {
    public:
        virtual void Reset() = 0;

        /* all scoring systems need to be able to respond to all judgments! */
        virtual void Update(ScoreKeeperJudgment skj, bool use_w0) = 0;
        virtual long long GetCurrentScore(long long max_notes, bool use_w0) const = 0;
        virtual long long GetMaxScore(long long max_notes, bool use_w0) = 0;
    };
}