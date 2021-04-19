#include <rmath.h>
#include <game/scoring_systems/RaindropScoring.h>

void rd::ScoreSystemRank::Reset() {
    rank_w0_count = 0;
    rank_w1_count = 0;
    rank_w2_count = 0;
    rank_w3_count = 0;
}

void rd::ScoreSystemRank::Update(rd::ScoreKeeperJudgment skj, bool use_w0) {
    if (skj < SKJ_W0) return;
    if (!use_w0 && skj <= SKJ_W0) ++rank_w0_count;

    if ((!use_w0 && skj <= SKJ_W1) || (use_w0 && skj <= SKJ_W0)) ++rank_w1_count;
    if ((!use_w0 && skj <= SKJ_W2) || (use_w0 && skj <= SKJ_W1)) ++rank_w2_count;
    if (skj <= SKJ_W3) ++rank_w3_count;

    ++judged_notes;
}

long long rd::ScoreSystemRank::GetCurrentScore(long long int max_notes, bool use_w0) const {
    long long rank_w0_pts = std::max(rank_w0_count * 2 - judged_notes, 0LL);
    long long rank_w1_pts = std::max(rank_w1_count * 2 - judged_notes, 0LL);
    long long rank_w2_pts = std::max(rank_w2_count * 2 - judged_notes, 0LL);
    long long rank_w3_pts = std::max(rank_w3_count * 2 - judged_notes, 0LL);

    return rank_w0_pts + rank_w1_pts + rank_w2_pts + rank_w3_pts;
}

long long rd::ScoreSystemRank::GetMaxScore(long long int max_notes, bool use_w0) {
    return 0;
}

int rd::ScoreSystemRank::GetRank() const {
    auto rank_pts = GetCurrentScore(0, 0);
    if (rank_pts == judged_notes * 400 / 100) return 15;
    if (rank_pts >= judged_notes * 380 / 100) return 14;
    if (rank_pts >= judged_notes * 360 / 100) return 13;
    if (rank_pts >= judged_notes * 340 / 100) return 12;
    if (rank_pts >= judged_notes * 320 / 100) return 11;

    if (rank_pts >= judged_notes * 300 / 100) return 10;
    if (rank_pts >= judged_notes * 280 / 100) return 9;
    if (rank_pts >= judged_notes * 260 / 100) return 8;
    if (rank_pts >= judged_notes * 240 / 100) return 7;
    if (rank_pts >= judged_notes * 220 / 100) return 6;
    if (rank_pts >= judged_notes * 200 / 100) return 5;
    if (rank_pts >= judged_notes * 180 / 100) return 4;
    if (rank_pts >= judged_notes * 160 / 100) return 3;
    if (rank_pts >= judged_notes * 140 / 100) return 2;
    if (rank_pts >= judged_notes * 120 / 100) return 1;
    if (rank_pts >= judged_notes * 100 / 100) return 0;
    if (rank_pts >= judged_notes * 90 / 100) return -1;
    if (rank_pts >= judged_notes * 80 / 100) return -2;
    if (rank_pts >= judged_notes * 70 / 100) return -3;
    if (rank_pts >= judged_notes * 60 / 100) return -4;
    if (rank_pts >= judged_notes * 50 / 100) return -5;
    if (rank_pts >= judged_notes * 40 / 100) return -6;
    if (rank_pts >= judged_notes * 20 / 100) return -7;
    if (rank_pts > 0) return -8;
    return -9;
}

void rd::ScoreSystemExp::Reset() {
    exp_combo = 0;
    exp_combo_pts = 0;
    exp_hit_score = 0;
}

void rd::ScoreSystemExp::Update(rd::ScoreKeeperJudgment skj, bool use_w0) {
    if (use_w0) {
        switch (skj) {
            case SKJ_W0:
                exp_combo += 4;
                exp_hit_score += 2;
                break;
            case SKJ_W1:
                exp_combo += 3;
                exp_hit_score += 2;
                break;
            case SKJ_W2:
                exp_combo += 2;
                exp_hit_score += 1;
                break;
            case SKJ_W3:
                exp_combo += 1;
                exp_hit_score += 1;
                break;
            default:
                exp_combo = 0;
        }

    } else { /* w0 is off */
        switch (skj) {
            case SKJ_W0:
            case SKJ_W1:
                exp_combo += 4;
                exp_hit_score += 2;
                break;
            case SKJ_W2:
                exp_combo += 3;
                exp_hit_score += 2;
                break;
            case SKJ_W3:
                exp_combo += 2;
                exp_hit_score += 1;
                break;
            default:
                exp_combo = 0;
        }
    }

    exp_combo_pts += exp_combo;

    if (exp_combo > 96) exp_combo = 96;
}

long long rd::ScoreSystemExp::GetCurrentScore(long long int max_notes, bool use_w0) const {
    return 1.2e6 * exp_combo_pts / GetMaxComboPts(max_notes);
}

long long rd::ScoreSystemExp::GetMaxScore(long long int max_notes, bool use_w0) {
    return 0; /* TODO: implement */
}

long long rd::ScoreSystemExp::GetMaxComboPts(long long max_notes) const {
    if (max_notes < 25)
        return max_notes * (max_notes + 1) * 2;
    else
        return 1300 + (max_notes - 25) * 100;
}

long long rd::ScoreSystemExp3::GetCurrentScore(long long int max_notes, bool use_w0) const {
    return 0.8e6 * exp_combo_pts / GetMaxComboPts(max_notes) + 0.1e6 * exp_hit_score / max_notes;
}

long long rd::ScoreSystemExp3::GetMaxScore(long long int max_notes, bool use_w0) {
    return 0; /* TODO: implement */
}
