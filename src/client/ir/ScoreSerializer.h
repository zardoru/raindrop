//
// Created by silav on 22/04/2022.
//

#ifndef RAINDROP_SCORESERIALIZER_H
#define RAINDROP_SCORESERIALIZER_H

#include "../game/PlayscreenParameters.h"
#include <ChartGroup.h>

namespace StormIR {
    class BadScoreType : public std::exception {
    public:
        BadScoreType() = default;
    };

    using nlohmann::json;

    json serialize_score(const otoworm::ChartGroup *chart_group, const otoworm::Chart *chart,
                                 const size_t index, const rd::ScoreKeeper &keeper, const PlayscreenParameters &options);

    json serialize_song_information(const otoworm::ChartGroup *chart_group);

    json serialize_difficulty_information(const otoworm::ChartGroup *chart_group, const otoworm::Chart *chart);

    json serialize_options(const PlayscreenParameters &parameters);

    json serialize_score_detail(const rd::ScoreKeeper &keeper, const rd::ScoreType type, const rd::LifeType gaugeType);
}

#endif //RAINDROP_SCORESERIALIZER_H
