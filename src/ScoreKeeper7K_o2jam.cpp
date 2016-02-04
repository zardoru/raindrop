#include "pch.h"

#include "ScoreKeeper7K.h"

int ScoreKeeper7K::getO2Judge(ScoreKeeperJudgment j)
{
    // if we've got a pill we may transform bads into cools
    if (j == SKJ_W1)
        coolcombo++;
    else
    {
        coolcombo = 0;
        if ((j == SKJ_W3) && pills) // transform this into a cool
        {
            pills--;
            j = SKJ_W1;
        }
    }

    if (coolcombo && coolcombo % 15) // every 15 cools you gain a pill, up to 5.
        pills = std::min(pills + 1, 5);

    return j;
}

void ScoreKeeper7K::update_o2(ScoreKeeperJudgment j)
{
    if (j == SKJ_W1)
    {
        jam_jchain += 2;
    }
    else if (j == SKJ_W2)
        jam_jchain += 1;
    else
    {
        jam_jchain = 0;
        // source says jam combo gets reset too, so ok..
        jams = 0;
    }

    // every 50 jam points you get a jam
    if (jam_jchain >= 50)
    {
        jam_jchain -= 50;
        jams++;
    }

    int weight;

    switch (j)
    {
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

void ScoreKeeper7K::setO2LifebarRating(int difficulty)
{
    difficulty = Clamp(difficulty, 0, 2);

    lifebar_o2jam = 1;

    // Thanks to Entozer for giving this information.
    switch (difficulty)
    {
    case 0: // EX
        lifebar_o2jam_increment = (19.0 / 20.0) / 290.0;
        lifebar_o2jam_decrement = 1.0 / 20.0;
        lifebar_o2jam_decrement_bad = 1.0 / 100.0;
        break;
    case 1: // NX
        lifebar_o2jam_increment = (24.0 / 25.0) / 470.0;
        lifebar_o2jam_decrement = 1.0 / 25.0;
        lifebar_o2jam_decrement_bad = 1.0 / 144.0;
        break;
    case 2: // HX
        lifebar_o2jam_increment = (33.0 / 34.0) / 950.0;
        lifebar_o2jam_decrement = 1.0 / 34.0;
        lifebar_o2jam_decrement_bad = 1.0 / 200.0;
        break;
    }
}

bool ScoreKeeper7K::usesO2()
{
    return use_bbased;
}

int ScoreKeeper7K::getCoolCombo()
{
    return coolcombo;
}

uint8_t ScoreKeeper7K::getPills()
{
    return pills;
}