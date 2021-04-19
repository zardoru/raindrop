#include <rmath.h>
#include <game/scoring_systems/OsumaniaScoring.h>

void rd::ScoreSystemOsuMania::Reset() {
    osu_points = 0;
    bonus_counter = 100;
    osu_bonus_points = 0;
}

void rd::ScoreSystemOsuMania::Update(rd::ScoreKeeperJudgment skj, bool use_w0) {
    int osu_bonus_multiplier = 0;

    if (skj < SKJ_W0 || skj > SKJ_MISS) return;

    switch (skj) {
        case SKJ_W0:
        case SKJ_W1:
            osu_points += 320;
            osu_bonus_multiplier = 32;
            bonus_counter = std::min(100, bonus_counter + 2);
            break;
        case SKJ_W2:
            osu_points += 300;
            osu_bonus_multiplier = 32;
            bonus_counter = std::min(100, bonus_counter + 1);
            break;
        case SKJ_W3:
            osu_points += 200;
            osu_bonus_multiplier = 16;
            bonus_counter = std::max(0, bonus_counter - 8);
            break;
        case SKJ_W4:
            osu_points += 100;
            osu_bonus_multiplier = 8;
            bonus_counter = std::max(0, bonus_counter - 24);
            break;
        case SKJ_W5:
            osu_points += 50;
            osu_bonus_multiplier = 4;
            bonus_counter = std::max(0, bonus_counter - 44);
            break;
        case SKJ_MISS:
            osu_points += 0;
            bonus_counter = 0;
            break;
        default:
            break;
    }

    osu_bonus_points += osu_bonus_multiplier * sqrt(double(bonus_counter));
}

long long rd::ScoreSystemOsuMania::GetCurrentScore(long long int max_notes, bool use_w0) const {
    return 500000 * ((osu_points + osu_bonus_points) / (max_notes * 320));
}

long long rd::ScoreSystemOsuMania::GetMaxScore(long long int max_notes, bool use_w0) {
    return 1000000;
}

void rd::ScoreSystemOsuManiaAccuracy::Reset() {
    osu_accuracy = 100;
}

void rd::ScoreSystemOsuManiaAccuracy::Update(rd::ScoreKeeperJudgment skj, bool use_w0) {

    switch (skj) {
        case SKJ_W0:
        case SKJ_W1:
            osu_accuracy += 300;
            break;
        case SKJ_W2:
            osu_accuracy += 300;
            break;
        case SKJ_W3:
            osu_accuracy += 200;
            break;
        case SKJ_W4:
            osu_accuracy += 100;
            break;
        case SKJ_W5:
            osu_accuracy += 50;
            break;
        default:
            break;
    }

}

long long rd::ScoreSystemOsuManiaAccuracy::GetCurrentScore(long long int max_notes, bool use_w0) const {
    return osu_accuracy * 100 / 3 / max_notes;
}

long long rd::ScoreSystemOsuManiaAccuracy::GetMaxScore(long long int max_notes, bool use_w0) {
    return 10000; // 100 and 2 decimals
}
