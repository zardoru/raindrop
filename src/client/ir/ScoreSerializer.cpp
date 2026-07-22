#include <string>
#include <filesystem>
#include <game/ScoreKeeper.h>
#include <json.hpp>
#include "ScoreSerializer.h"
#include <text_and_file_util.h>

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

json StormIR::serialize_score(const otoworm::ChartGroup *chart_group, const otoworm::Chart *chart,
                             const size_t index, const rd::ScoreKeeper &keeper,
                             const PlayscreenParameters &options) {
    json ret;

    // version
    ret["ver"] = 1;

    // difficulty index
    ret["diff_index"] = index;

    ret["song"] = serialize_song_information(chart_group);
    ret["diff"] = serialize_difficulty_information(chart_group, chart);
    ret["options"] = serialize_options(options);
    ret["detail"] = serialize_score_detail(keeper, options.get_scoring_type(),
                                         static_cast<const rd::LifeType>(options.gauge_type));

    // score type
    switch (options.get_scoring_type()) {
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
            ret["score_type"] = scoreTypeMapper.at(options.get_scoring_type());
            break;
        default:
            throw BadScoreType();
    }

    ret["system_type"] = systemTypeMapper.at(options.system_type);
    return ret;
}

json StormIR::serialize_song_information(const otoworm::ChartGroup *chart_group) {
    return nlohmann::json() = {
            {"title",    chart_group ? chart_group->title : ""},
            {"subtitle", chart_group ? chart_group->subtitle : ""},
            {"artist",   chart_group ? chart_group->artist : ""}
    };
}

json StormIR::serialize_difficulty_information(const otoworm::ChartGroup *chart_group, const otoworm::Chart *chart) {
    std::filesystem::path chart_path;
    if (chart && chart->meta)
        chart_path = chart->meta->path;
    if (chart_group && !chart_path.empty() && chart_path.is_relative())
        chart_path = chart_group->path / chart_path;

    return nlohmann::json() = {
            {"sha256",    chart_path.empty() ? "" : otoworm::util::get_sha256_for_file(chart_path)},
            {"name",      chart && chart->meta ? chart->meta->name : ""},
            {"charter",   chart && chart->meta ? chart->meta->author : ""},
            {"playlevel", chart ? chart->level : 0},
            {"channels",  chart ? chart->channels : 0}
    };
}

json StormIR::serialize_options(const PlayscreenParameters &parameters) {
    std::vector<std::string> opts;

    if (parameters.no_fail) opts.emplace_back("nofail");
    if (parameters.rate != 1.0) opts.push_back(otoworm::util::format("rate:%.2f", parameters.rate));
    if (parameters.use_w0) opts.emplace_back("w0");
    if (parameters.random) opts.push_back(otoworm::util::format("seed:%d", parameters.seed));
    if (parameters.upscroll) opts.emplace_back("scroll:up");

    opts.push_back(otoworm::util::format("g:%s", gaugeTypeMapper.at(parameters.gauge_type).c_str()));

    return json() = opts;
}

json StormIR::serialize_score_detail(const rd::ScoreKeeper &keeper, const rd::ScoreType type, const rd::LifeType gaugeType) {
    json ret;
    ret["judge"] = {
            {"w0" , keeper.get_judgment_count(rd::SKJ_W0)},
            {"w1" , keeper.get_judgment_count(rd::SKJ_W1)},
            {"w2" , keeper.get_judgment_count(rd::SKJ_W2)},
            {"w3" , keeper.get_judgment_count(rd::SKJ_W3)},
            {"w4" , keeper.get_judgment_count(rd::SKJ_W4)},
            {"w5" , keeper.get_judgment_count(rd::SKJ_W5)},
            {"miss" , keeper.get_judgment_count(rd::SKJ_MISS)},
            {"mine" , keeper.get_judgment_count(rd::SKJ_MINE)}
    };

    ret["score"] = keeper.get_score(type);
    ret["gauge"] = keeper.get_lifebar_amount(gaugeType);
    ret["pass"] = !keeper.is_stage_failed(gaugeType);

    if (type == rd::ST_OSUMANIA)
        ret["om_acc"] = keeper.get_score(rd::ST_OSUMANIA_ACC);

    return ret;
}
