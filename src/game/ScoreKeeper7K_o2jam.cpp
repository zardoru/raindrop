#include "rmath.h"

#include <game/GameConstants.h>
#include <game/ScoreKeeper7K.h>

namespace rd {
    int ScoreKeeper::getO2Judge(ScoreKeeperJudgment j) {
        // if we've got a pill we may transform bads into cools
        if (j == SKJ_W1)
            coolcombo++;
        else {
            coolcombo = 0;
            if ((j == SKJ_W3) && pills) // transform this into a cool
            {
                pills--;
                j = SKJ_W1;
            }
        }

        if (coolcombo && (coolcombo % 15 == 0)) // every 15 cools you gain a pill, up to 5.
            pills = std::min(pills + 1, 5);

        return j;
    }

    void ScoreKeeper::update_o2(ScoreKeeperJudgment j) {
        if (j == SKJ_W1) {
            jam_jchain += 2;
        } else if (j == SKJ_W2)
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

        switch (j) {
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



    bool ScoreKeeper::usesO2() const {
        return use_o2jam;
    }

    int ScoreKeeper::getCoolCombo() const {
        return coolcombo;
    }

    uint8_t ScoreKeeper::getPills() const {
        return pills;
    }
}