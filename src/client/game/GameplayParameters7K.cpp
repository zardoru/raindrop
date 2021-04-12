#include <game/PlayerChartState.h>
#include <game/VSRGMechanics.h>
#include <game/ScoreKeeper7K.h>
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


void PlayscreenParameters::UpdateHidden(double JudgeY)
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
    if (HiddenMode)
    {
        float pfCenter;

        Hidden.TransitionSize = HiddenSize * PLAYFIELD_SIZE / ScreenHeight;

        if (Upscroll)
        {
            pfCenter = toYRange(ScreenHeight - JudgeY + Threshold * PLAYFIELD_SIZE);
            Center = pfCenter;


            // Invert Hidden Mode.
            if (HiddenMode == HM_SUDDEN) Hidden.Mode = HM_HIDDEN;
            else if (HiddenMode == HM_HIDDEN) Hidden.Mode = HM_SUDDEN;
            else Hidden.Mode = (EHiddenMode)HiddenMode;
        }
        else
        {
            // pfCenter = Center;
            // Center = pfCenter;
            Hidden.Mode = (EHiddenMode)HiddenMode;
        }

        Hidden.CenterSize = FLSize * PLAYFIELD_SIZE / ScreenHeight;
    }
}

ScoreType PlayscreenParameters::GetScoringType() const
{
    if (SystemType == TI_BMS || SystemType == TI_RDAC) {
        return ST_EX;
    }

    if (SystemType == TI_O2JAM) {
        return ST_O2JAM;
    }

    if (SystemType == TI_OSUMANIA) {
        return ST_OSUMANIA;
    }

    if (SystemType == TI_STEPMANIA) {
        return ST_EX;
    }

    if (SystemType == TI_RAINDROP) {
        return ST_EXP3;
    }

    if (SystemType == TI_LR2) {
        return ST_LR2; /* most people use EX but hey... */
    }

    return ST_EX;
}

int PlayscreenParameters::GetHiddenMode() const
{
    return Hidden.Mode;
}

float PlayscreenParameters::GetHiddenCenter() const
{
    return Hidden.Center;
}

float PlayscreenParameters::GetHiddenTransitionSize() const
{
    return Hidden.TransitionSize;
}

float PlayscreenParameters::GetHiddenCenterSize() const
{
    return Hidden.CenterSize;
}

int PlayscreenParameters::GetSeed() const
{
    return Seed;
}

void PlayscreenParameters::SetSeed(int seed)
{
    Seed = seed;
    IsSeedSet = true;
}

void PlayscreenParameters::ResetSeed()
{
    IsSeedSet = false;
}

void deserialize(PlayscreenParameters &out, nlohmann::json json)
{
    out.Upscroll = json["upscroll"];
    out.NoFail = json["nofail"];
    out.HiddenMode = json["hidden"];
    out.Rate = json["rate"];
    out.UserSpeedMultiplier = json["userspeed"];
    out.Random = json["random"];
    out.GaugeType = json["gauge"];
    out.SystemType = json["system"];
    out.GreenNumber = json["isGreenNumber"];
    out.UseW0 = json["W0"];
    // future use:
    // ScoringType = json["score"];

    if (out.Random) {
        out.SetSeed(json["seed"]);
    }
}

nlohmann::json serialize(const PlayscreenParameters &in)
{
    nlohmann::json ret;
    ret["upscroll"] = in.Upscroll;
    ret["nofail"] = in.NoFail;
    ret["hidden"] = in.HiddenMode;
    ret["rate"] = in.Rate;
    ret["userspeed"] = in.UserSpeedMultiplier;
    ret["random"] = in.Random;
    ret["gauge"] = in.GaugeType;
    ret["system"] = in.SystemType;
    ret["score"] = in.GetScoringType();
    ret["isGreenNumber"] = in.GreenNumber;
    ret["W0"] = in.UseW0;

    if (in.Random && in.IsSeedSet) {
        ret["seed"] = in.Seed;
    }

    if (in.Random && !in.IsSeedSet)
        Log::LogPrintf("Warning: serializing with random set, but no seed\n");

    return ret;
}