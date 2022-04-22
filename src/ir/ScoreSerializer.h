//
// Created by silav on 22/04/2022.
//

#ifndef RAINDROP_SCORESERIALIZER_H
#define RAINDROP_SCORESERIALIZER_H

#include "../client/game/PlayscreenParameters.h"

namespace StormIR {
    class BadScoreType : public std::exception {
    public:
        BadScoreType() = default;
    };

    using nlohmann::json;

    json SerializeScore(const rd::Song *pSong, const rd::Difficulty *pDifficulty,
                                 const size_t index, const rd::ScoreKeeper &keeper, const PlayscreenParameters &options);

    json SerializeSongInformation(const rd::Song *pSong);

    json SerializeDifficultyInformation(const rd::Difficulty *pDifficulty);

    json SerializeOptions(const PlayscreenParameters &parameters);

    json SerializeScoreDetail(const rd::ScoreKeeper &keeper, const rd::ScoreType type, const rd::LifeType gaugeType);
}

#endif //RAINDROP_SCORESERIALIZER_H
