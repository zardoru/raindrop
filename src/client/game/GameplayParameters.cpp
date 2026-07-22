#include <ProcessedChart.h>
#include <game/VSRGMechanics.h>
#include <game/ScoreKeeper.h>
#include <game/NoteTransformations.h>

#include "Logging.h"
#include "../structure/Configuration.h"
#include "PlayscreenParameters.h"

#include <json.hpp>
#include "../serialize/PlayscreenParameters.h"

/* Vertical Space for a Measure.
    A single 4/4 measure takes all of the playing field.
    Increasing this will decrease multiplier resolution. */
extern SkinMetric PLAYFIELD_SIZE;
extern SkinMetric UNITS_PER_MEASURE;

using namespace rd;


void PlayscreenParameters::update_hidden(double judge_y)
{
    /*
    Given the top of the screen being 1, the bottom being -1
    calculate the range for which the current hidden mode is defined.
    */
    CfgVar HiddenSize("HiddenSize", "Hidden");
    CfgVar FLSize("FlashlightSize", "Hidden");
    CfgVar Threshold("Threshold", "Hidden");

    auto toYRange = [](float x) {
        x /= ScreenHeight; // [0,768] -> [0,1]
        x *= -2; // [0,1] -> [0, -2]
        x += 1; // [1, -1]
        return x;
    };

    float Center = toYRange(Threshold * PLAYFIELD_SIZE);

    // Hidden calc
    if (hidden_mode)
    {
        float pfCenter;

        hidden_.transition_size = HiddenSize * PLAYFIELD_SIZE / ScreenHeight;

        if (upscroll)
        {
            pfCenter = toYRange(ScreenHeight - judge_y + Threshold * PLAYFIELD_SIZE);
            Center = pfCenter;


            // Invert Hidden Mode.
            if (hidden_mode == HM_SUDDEN) hidden_.mode = HM_HIDDEN;
            else if (hidden_mode == HM_HIDDEN) hidden_.mode = HM_SUDDEN;
            else hidden_.mode = (EHiddenMode)hidden_mode;
        }
        else
        {
            // pfCenter = Center;
            // Center = pfCenter;
            hidden_.mode = (EHiddenMode)hidden_mode;
        }

        hidden_.center_size = FLSize * PLAYFIELD_SIZE / ScreenHeight;
    }
}

ScoreType PlayscreenParameters::get_scoring_type() const
{
    if (system_type == TI_BMS || system_type == TI_RDAC) {
        return ST_EX;
    }

    if (system_type == TI_O2JAM) {
        return ST_O2JAM;
    }

    if (system_type == TI_OSUMANIA) {
        return ST_OSUMANIA;
    }

    if (system_type == TI_STEPMANIA) {
        return ST_EX;
    }

    if (system_type == TI_RAINDROP) {
        return ST_EXP3;
    }

    if (system_type == TI_LR2) {
        return ST_LR2; /* most people use EX but hey... */
    }

    return ST_EX;
}

int PlayscreenParameters::get_hidden_mode() const
{
    return hidden_.mode;
}

float PlayscreenParameters::get_hidden_center() const
{
    return hidden_.center;
}

float PlayscreenParameters::get_hidden_transition_size() const
{
    return hidden_.transition_size;
}

float PlayscreenParameters::get_hidden_center_size() const
{
    return hidden_.center_size;
}

int PlayscreenParameters::get_seed() const
{
    return seed;
}

void PlayscreenParameters::set_seed(int seed)
{
    seed = seed;
    is_seed_set = true;
}

void PlayscreenParameters::reset_seed()
{
    is_seed_set = false;
}

void deserialize(PlayscreenParameters &out, nlohmann::json json)
{
    out.upscroll = json["upscroll"];
    out.no_fail = json["nofail"];
    out.hidden_mode = json["hidden"];
    out.rate = json["rate"];
    out.user_speed_multiplier = json["userspeed"];
    out.random = json["random"];
    out.gauge_type = json["gauge"];
    out.system_type = json["system"];
    out.green_number = json["isGreenNumber"];
    out.use_w0 = json["W0"];
    // future use:
    // ScoringType = json["score"];

    if (out.random) {
        out.set_seed(json["seed"]);
    }
}

nlohmann::json serialize(const PlayscreenParameters &in)
{
    nlohmann::json ret;
    ret["upscroll"] = in.upscroll;
    ret["nofail"] = in.no_fail;
    ret["hidden"] = in.hidden_mode;
    ret["rate"] = in.rate;
    ret["userspeed"] = in.user_speed_multiplier;
    ret["random"] = in.random;
    ret["gauge"] = in.gauge_type;
    ret["system"] = in.system_type;
    ret["score"] = in.get_scoring_type();
    ret["isGreenNumber"] = in.green_number;
    ret["W0"] = in.use_w0;

    if (in.random && in.is_seed_set) {
        ret["seed"] = in.seed;
    }

    if (in.random && !in.is_seed_set)
        Log::LogPrintf("Warning: serializing with random set, but no seed\n");

    return ret;
}