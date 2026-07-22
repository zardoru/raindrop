//
// Created by silav on 21/04/2022.
//

#include <queue>
#include <string>
#include <game/ScoreKeeper.h>
#include "game/Song.h"
#include <memory>
#include <cpr/cpr.h>
#include <json.hpp>

#include "../game/PlayscreenParameters.h"
#include "../game/Replay.h"
#include "StormIR.h"
#include "ScoreSerializer.h"
#include "Logging.h"


using nlohmann::json;

const std::string backendUrl = "https://parseapi.back4app.com";

namespace StormIR {

    class StormIRImpl {
    public:
        std::string _sessionToken;
        std::string appid;
        std::string clientkey;

        // create the session object for a parse call
    public:

        std::unique_ptr<cpr::Session> ApiRequest(std::string action) {
            using namespace cpr;
            auto sess = std::make_unique<cpr::Session>();
            sess->SetUrl(cpr::Url{backendUrl + "/" + action});
            sess->SetHeader({
                   {"X-Parse-Application-Id", appid},
                   {"X-Parse-Client-Key",     clientkey}
           });

            // add logged in request
            if (!_sessionToken.empty())
                sess->UpdateHeader({
                                          {"X-Parse-Session-Token", _sessionToken}
                                  });


            return sess;
        }
    };

    StormIR::StormIR(std::string appid, std::string clientkey) {
        _impl = std::make_unique<StormIRImpl>();

        _impl->appid = appid;
        _impl->clientkey = clientkey;
    }

    StormIR::~StormIR() = default;

    bool StormIR::login(std::string username, std::string password) {
        if (is_connected())
            return true;

        auto s = _impl->ApiRequest("login");
        s->SetParameters({
                {"username", username},
                {"password", password}
        });

        auto r = s->Get();
        if (r.status_code != 200) {
            // failed to log in
            auto err = json::parse(r.text);
            last_error = err["error"];
            return false;
        }

        auto sess = json::parse(r.text);
        _impl->_sessionToken = sess["sessionToken"];

#if _DEBUG
        Log::LogPrintf("Sess Key: %s", _impl->_sessionToken.c_str());
#endif

        return true;
    }

    bool StormIR::is_connected() {
        return !_impl->_sessionToken.empty();
    }

    bool StormIR::submit_score(
            const otoworm::ChartGroup *chart_group,
            const otoworm::Chart *chart,
            const Replay &replay,
            const rd::ScoreKeeper &score
    ) {
        if (!is_connected())
            return false;

        // most of the hard work is done in this function
        auto j = serialize_score(
                chart_group,
                chart,
                replay.get_difficulty_index(),
                score,
                replay.get_effective_parameters()
        );
        auto s = _impl->ApiRequest("functions/submitScore");
        auto b = j.dump();
        s->UpdateHeader({{"Content-Type", "application/json"}});
        s->SetBody(cpr::Body{b});

        auto r = s->Post();
        auto t = r.text;
        auto errJson = json::parse(r.text);
        auto hasError = !errJson["code"].is_null() && !errJson["message"].is_null();
        if (r.error || hasError) {
            last_error = r.text;
            return false;
        }

        return true;
    }

    void StormIR::fetch_personal_score() {

    }

    std::string StormIR::get_session_token() {
        return _impl->_sessionToken;
    }

    std::string StormIR::get_last_error() {
        return last_error;
    }
}
