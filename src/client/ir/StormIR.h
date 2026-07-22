//
// Created by silav on 21/04/2022.
//

#ifndef RAINDROP_STORMIR_H
#define RAINDROP_STORMIR_H

#include <ChartGroup.h>

namespace StormIR {

    class StormIRImpl;

    class StormIR {
    private:
        std::unique_ptr<StormIRImpl> _impl;
        std::string last_error;
    public:
        StormIR(std::string appid, std::string clientkey);

        ~StormIR();

        bool login(std::string username, std::string password);

        bool is_connected();

        std::string get_session_token();

        std::string get_last_error();

        void fetch_personal_score();

        bool
        submit_score(const otoworm::ChartGroup *chart_group, const otoworm::Chart *chart, const Replay& replay,
                    const rd::ScoreKeeper &score);
    };

}

#endif //RAINDROP_STORMIR_H
