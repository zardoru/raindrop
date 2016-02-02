#include "pch.h"

#include "ScoreKeeper7K.h"

void ScoreKeeper7K::update_ranks(ScoreKeeperJudgment judgment) {
    if (!use_w0_for_ex2 && judgment <= SKJ_W0) ++rank_w0_count;

    if (!use_w0_for_ex2 && judgment <= SKJ_W1 || use_w0_for_ex2 && judgment <= SKJ_W0) ++rank_w1_count;
    if (!use_w0_for_ex2 && judgment <= SKJ_W2 || use_w0_for_ex2 && judgment <= SKJ_W1) ++rank_w2_count;
    if (judgment <= SKJ_W3) ++rank_w3_count;

    long long rank_w0_pts = std::max(rank_w0_count * 2 - total_notes, 0LL);
    long long rank_w1_pts = std::max(rank_w1_count * 2 - total_notes, 0LL);
    long long rank_w2_pts = std::max(rank_w2_count * 2 - total_notes, 0LL);
    long long rank_w3_pts = std::max(rank_w3_count * 2 - total_notes, 0LL);

    rank_pts = rank_w0_pts + rank_w1_pts + rank_w2_pts + rank_w3_pts;
}

int ScoreKeeper7K::getRank() {
    // find some way to streamline this.

    if (rank_pts == total_notes * 400 / 100) return 15;
    if (rank_pts >= total_notes * 380 / 100) return 14;
    if (rank_pts >= total_notes * 360 / 100) return 13;
    if (rank_pts >= total_notes * 340 / 100) return 12;
    if (rank_pts >= total_notes * 320 / 100) return 11;

    if (rank_pts >= total_notes * 300 / 100) return 10;
    if (rank_pts >= total_notes * 280 / 100) return 9;
    if (rank_pts >= total_notes * 260 / 100) return 8;
    if (rank_pts >= total_notes * 240 / 100) return 7;
    if (rank_pts >= total_notes * 220 / 100) return 6;
    if (rank_pts >= total_notes * 200 / 100) return 5;
    if (rank_pts >= total_notes * 180 / 100) return 4;
    if (rank_pts >= total_notes * 160 / 100) return 3;
    if (rank_pts >= total_notes * 140 / 100) return 2;
    if (rank_pts >= total_notes * 120 / 100) return 1;
    if (rank_pts >= total_notes * 100 / 100) return 0;
    if (rank_pts >= total_notes * 90 / 100) return -1;
    if (rank_pts >= total_notes * 80 / 100) return -2;
    if (rank_pts >= total_notes * 70 / 100) return -3;
    if (rank_pts >= total_notes * 60 / 100) return -4;
    if (rank_pts >= total_notes * 50 / 100) return -5;
    if (rank_pts >= total_notes * 40 / 100) return -6;
    if (rank_pts >= total_notes * 20 / 100) return -7;
    if (rank_pts > 0) return -8;
    return -9;
}