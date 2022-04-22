#include <string>
#include <game/ScoreKeeper7K.h>
#include <json.hpp>
#include "game/Song.h"
#include "ScoreSerializer.h"
#include "../client/game/PlayscreenParameters.h"
#include "TextAndFileUtil.h"

using nlohmann::json;

const std::map<rd::ScoreType, std::string> scoreTypeMapper = {
        // the commented out types are not system scores
        // {rd::ST_RANK,         "rdrank"},
        {rd::ST_EX,       "ex"},
        {rd::ST_DP,       "dp"},
        {rd::ST_IIDX,     "iidx"},
        {rd::ST_LR2,      "lr2"},
        {rd::ST_SCORE,    "scr"},
        {rd::ST_OSUMANIA, "o!m"},
        // {ST_JB2,          "jb2"},
        // {rd::ST_OSUMANIA_ACC, "o!m-acc"},
        {rd::ST_EXP,      "exp"},
        {rd::ST_EXP3,     "exp3"},
        {rd::ST_O2JAM,    "o2jam"},
        // {ST_COMBO,        "combo"},
        // {ST_MAX_COMBO,    "maxcombo"},
        // {ST_NOTES_HIT,    "nh%"},
};

const std::map<int32_t, std::string> gaugeTypeMapper = {
        { rd::LT_AUTO,            "unk" },
        { rd::LT_GROOVE,          "groove" },
        { rd::LT_SURVIVAL,        "survival" },
        { rd::LT_EXHARD,          "exh" },
        { rd::LT_DEATH,           "death" },
        { rd::LT_EASY,            "ez" },
        { rd::LT_STEPMANIA,       "smj4" },
        { rd::LT_NORECOV,         "norecov" },
        { rd::LT_O2JAM,           "o2jam" },
        { rd::LT_OSUMANIA,        "o!m" },
        { rd::LT_BATTERY,         "battery" },
        { rd::LT_LR2_ASSIST,      "assist_lr2" },
        { rd::LT_LR2_EASY,        "easy_lr2" },
        { rd::LT_LR2_NORMAL,      "normal_lr2" },
        { rd::LT_LR2_HARD,        "hard_lr2" },
        { rd::LT_LR2_EXHARD,      "exh_lr2" },
        { rd::LT_LR2_HAZARD,      "hazard_lr2" },
        { rd::LT_LR2_CLASS,       "class_lr2" },
        { rd::LT_LR2_EXCLASS,     "exc_lr2" },
        { rd::LT_LR2_EXHARDCLASS, "exhc_lr2 "}
};

const std::map<int, std::string> systemTypeMapper = {
        { rd::TI_BMS ,       "rdbms" }    ,
        { rd::TI_OSUMANIA ,  "om" }       ,
        { rd::TI_O2JAM ,     "o2jam" }    ,
        { rd::TI_STEPMANIA , "sm" }       ,
        { rd::TI_RAINDROP ,  "rdbms" }    ,
        { rd::TI_RDAC ,      "rdbms-ac" } ,
        { rd::TI_LR2 ,       "lr2" }      ,

};

json StormIR::SerializeScore(const rd::Song *pSong, const rd::Difficulty *pDifficulty,
                             const size_t index, const rd::ScoreKeeper &keeper,
                             const PlayscreenParameters &options) {
    json ret;

    // version
    ret["ver"] = 1;

    // difficulty index
    ret["diff_index"] = index;

    ret["song"] = SerializeSongInformation(pSong);
    ret["diff"] = SerializeDifficultyInformation(pDifficulty);
    ret["options"] = SerializeOptions(options);
    ret["detail"] = SerializeScoreDetail(keeper, options.GetScoringType(),
                                         static_cast<const rd::LifeType>(options.GaugeType));

    // score type
    switch (options.GetScoringType()) {
        // must match scoreTypeMapper definition above
        case rd::ST_EX:
        case rd::ST_DP:
        case rd::ST_IIDX:
        case rd::ST_LR2:
        case rd::ST_SCORE:
        case rd::ST_OSUMANIA:
        case rd::ST_EXP:
        case rd::ST_EXP3:
        case rd::ST_O2JAM:
            ret["score_type"] = scoreTypeMapper.at(options.GetScoringType());
            break;
        default:
            throw BadScoreType();
    }

    ret["system_type"] = systemTypeMapper.at(options.SystemType);
    return ret;
}

json StormIR::SerializeSongInformation(const rd::Song *pSong) {
    return nlohmann::json() = {
            {"title",    pSong->Title},
            {"subtitle", pSong->Subtitle},
            {"artist",   pSong->Artist}
    };
}

json StormIR::SerializeDifficultyInformation(const rd::Difficulty *pDifficulty) {
    return nlohmann::json() = {
            {"sha256",    Utility::GetSha256ForFile(pDifficulty->Filename)},
            {"name",      pDifficulty->Name},
            {"charter",   pDifficulty->Author},
            {"playlevel", pDifficulty->Level},
            {"channels",  pDifficulty->Channels}
    };
}

json StormIR::SerializeOptions(const PlayscreenParameters &parameters) {
    std::vector<std::string> opts;

    if (parameters.NoFail) opts.emplace_back("nofail");
    if (parameters.Rate != 1.0) opts.push_back(Utility::Format("rate:%.2f", parameters.Rate));
    if (parameters.UseW0) opts.emplace_back("w0");
    if (parameters.Random) opts.push_back(Utility::Format("seed:%d", parameters.Seed));
    if (parameters.Upscroll) opts.emplace_back("scroll:up");

    opts.push_back(Utility::Format("g:%s", gaugeTypeMapper.at(parameters.GaugeType).c_str()));

    return json() = opts;
}

json StormIR::SerializeScoreDetail(const rd::ScoreKeeper &keeper, const rd::ScoreType type, const rd::LifeType gaugeType) {
    json ret;
    ret["judge"] = {
            {"w0" , keeper.getJudgmentCount(rd::SKJ_W0)},
            {"w1" , keeper.getJudgmentCount(rd::SKJ_W1)},
            {"w2" , keeper.getJudgmentCount(rd::SKJ_W2)},
            {"w3" , keeper.getJudgmentCount(rd::SKJ_W3)},
            {"w4" , keeper.getJudgmentCount(rd::SKJ_W4)},
            {"w5" , keeper.getJudgmentCount(rd::SKJ_W5)},
            {"miss" , keeper.getJudgmentCount(rd::SKJ_MISS)},
            {"mine" , keeper.getJudgmentCount(rd::SKJ_MINE)}
    };

    ret["score"] = keeper.getScore(type);
    ret["gauge"] = keeper.getLifebarAmount(gaugeType);
    ret["pass"] = !keeper.isStageFailed(gaugeType);

    if (type == rd::ST_OSUMANIA)
        ret["om_acc"] = keeper.getScore(rd::ST_OSUMANIA_ACC);

    return ret;
}
