#include <rmath.h>
#include <game/scoring_systems/BMSScoring.h>

void rd::ScoreSystemBMS::Reset() {
    bms_combo = -1;
    bms_combo = 0;
    bms_combo_pts = 0;
    bms_dance_pts = 0;
}

void rd::ScoreSystemBMS::Update(rd::ScoreKeeperJudgment skj, bool use_w0) {
    if (skj == SKJ_NONE || skj > SKJ_MISS) return;

    if (use_w0) {
        switch (skj) {
            case SKJ_W0:
                bms_dance_pts += 15;
                break;
            case SKJ_W1:
                bms_dance_pts += 10;
                break;
            case SKJ_W2:
                bms_dance_pts += 2;
                break;
            default:
                bms_combo = -1;
                break;
        }
    } else {
        switch (skj) {
            case SKJ_W1:
                bms_dance_pts += 15;
                break;
            case SKJ_W2:
                bms_dance_pts += 10;
                break;
            case SKJ_W3:
                bms_dance_pts += 2;
                break;
            default:
                bms_combo = -1;
                break;
        }
    }

    bms_combo = std::min(10LL, bms_combo + 1);
    bms_combo_pts += bms_combo;
}

long long rd::ScoreSystemBMS::GetCurrentScore(long long int max_notes, bool use_w0) const {
    return 50000 * bms_combo_pts / GetMaxComboPts(max_notes)
           + 10000 * bms_dance_pts / max_notes;
}

long long rd::ScoreSystemBMS::GetMaxScore(long long int max_notes, bool use_w0) {
    return 1; /* TODO: implement i guess */
}

long long rd::ScoreSystemBMS::GetMaxComboPts(long long int notes) {
    if (notes < 10)
        return notes * (notes + 1) / 2;
    else
        return 55 + (notes - 10) * 10;
}

void rd::ScoreSystemLR2::Reset() {
    lr2_dance_pts = 0;
}

void rd::ScoreSystemLR2::Update(rd::ScoreKeeperJudgment skj, bool use_w0) {
    if (skj == SKJ_NONE || skj > SKJ_MISS) return;
    if (use_w0) {
        switch (skj) {
            case SKJ_W0:
                lr2_dance_pts += 10;
                break;
            case SKJ_W1:
                lr2_dance_pts += 5;
                break;
            case SKJ_W2:
                lr2_dance_pts += 1;
                break;
            default:
                break;
        }
    } else {
        switch (skj) {
            case SKJ_W1:
                lr2_dance_pts += 10;
                break;
            case SKJ_W2:
                lr2_dance_pts += 5;
                break;
            case SKJ_W3:
                lr2_dance_pts += 1;
                break;
            default:
                break;
        }
    }
}

long long rd::ScoreSystemLR2::GetCurrentScore(long long int max_notes, bool use_w0) const {
    return 20000 * lr2_dance_pts / max_notes;
}

long long rd::ScoreSystemLR2::GetMaxScore(long long int max_notes, bool use_w0) {
    return 200000;
}

void rd::ScoreSystemEX::Reset() {
    ex_score = 0;
}

void rd::ScoreSystemEX::Update(rd::ScoreKeeperJudgment skj, bool use_w0) {
    if (skj == SKJ_NONE || skj > SKJ_MISS) return;
    if (use_w0) {
        switch (skj) {
            case SKJ_W0:
                ex_score += 2;
                break;
            case SKJ_W1:
                ex_score += 1;
                break;
            default:
                break;
        }
    } else {
        switch (skj) {
            case SKJ_W1:
                ex_score += 2;
                break;
            case SKJ_W2:
                ex_score += 1;
                break;
            default:
                break;
        }
    }
}

long long rd::ScoreSystemEX::GetCurrentScore(long long int max_notes, bool use_w0) const {
    return ex_score;
}

long long rd::ScoreSystemEX::GetMaxScore(long long int max_notes, bool use_w0) {
    return max_notes * 2;
}

rd::PacemakerType rd::ScoreSystemEX::GetRank(long long max_notes) const {
    static constexpr double thresholds[] = {8.0 / 9.0, 7.0 / 9.0, 6.0 / 9.0,
                           5.0 / 9.0, 4.0 / 9.0, 3.0 / 9.0, 2.0 / 9.0, 1.0 / 9.0,
                           0, -std::numeric_limits<double>::infinity()};

    double exps = double (ex_score) / double (max_notes) / 2.0;
    auto rank_index = 9;

    // see if the current ex score crosses the threshold for this BM rank
    for (auto i = 0; i < sizeof(thresholds) / sizeof(double); i++) {
        if (exps > thresholds[i]) {
            rank_index = i;
            break;
        }
    }

    switch (rank_index) {
        case 0:
            return PMT_AAA;
        case 1:
            return PMT_AA;
        case 2:
            return PMT_A;
        case 3:
            return PMT_B;
        case 4:
            return PMT_C;
        case 5:
            return PMT_D;
        case 6:
            return PMT_E;
        case 7:
        default:
            return PMT_F;
    }
}
