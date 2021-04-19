#include <rmath.h>
#include <game/scoring_systems/O2JamScoring.h>


void rd::ScoreSystemO2Jam::Reset() {
    coolcombo = 0;
    pills = 0;
    jams = 0;
    jam_jchain = 0;
    o2_score = 0;
}

void rd::ScoreSystemO2Jam::Update(rd::ScoreKeeperJudgment skj, bool use_w0) {
    if (skj > SKJ_MISS || skj <= -1) return;

    if (skj == SKJ_W1) {
        jam_jchain += 2;
    } else if (skj == SKJ_W2)
        jam_jchain += 1;
    else {
        jam_jchain = 0;
        // source says jam combo gets reset too, so ok..
        jams = 0;
    }

    // every 50 jam points you get a jam
    if (jam_jchain >= 50) {
        jam_jchain -= 50;
        jams++;
    }

    int weight;

    switch (skj) {
        case SKJ_W0:
        case SKJ_W1:
            weight = 200 + 10 * jams;
            break;
        case SKJ_W2:
            weight = 100 + 5 * jams;
            break;
        case SKJ_W3:
            weight = 4;
            break;
        case SKJ_MISS:
            weight = -10;
            break;
        default:
            weight = 0;
    }

    o2_score += weight;
    o2_score = std::max(static_cast<long long>(0), o2_score);
}

long long rd::ScoreSystemO2Jam::GetCurrentScore(long long int max_notes, bool use_w0) const {
    return o2_score;
}

long long rd::ScoreSystemO2Jam::GetMaxScore(long long int max_notes, bool use_w0) {
    return 0; /* TODO: h-how can I EVEN KNOW WHO EVEN WANTS TO DO THE MATHS */
}

rd::ScoreKeeperJudgment rd::ScoreSystemO2Jam::MutateJudgment(rd::ScoreKeeperJudgment skj) {
    // if we've got a pill we may transform bads into cools
    if (skj == SKJ_W1)
        coolcombo++;
    else {
        coolcombo = 0;
        if ((skj == SKJ_W3) && pills) // transform this into a cool
        {
            pills--;
            skj = SKJ_W1;
        }
    }

    if (coolcombo && (coolcombo % 15 == 0)) // every 15 cools you gain a pill, up to 5.
        pills = std::min(pills + 1, 5);

    return skj;
}

long long rd::ScoreSystemO2Jam::GetCoolCombo() const {
    return coolcombo;
}

long long rd::ScoreSystemO2Jam::GetJams() const {
    return jams;
}

char rd::ScoreSystemO2Jam::GetPills() const {
    return pills;
}

long long rd::ScoreSystemO2Jam::GetJamChain() const {
    return jam_jchain;
}
