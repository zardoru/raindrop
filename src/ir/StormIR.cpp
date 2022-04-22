//
// Created by silav on 21/04/2022.
//

#include <queue>
#include <string>
#include <game/ScoreKeeper7K.h>
#include "game/Song.h"
#include <memory>
#include <cpr/cpr.h>
#include <json.hpp>

#include "../client/game/PlayscreenParameters.h"
#include "../client/game/Replay7K.h"
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

        cpr::Session ApiRequest(std::string action) {
            using namespace cpr;
            cpr::Session sess;
            sess.SetUrl(cpr::Url{backendUrl + "/" + action});
            sess.SetHeader({
                   {"X-Parse-Application-Id", appid},
                   {"X-Parse-Client-Key",     clientkey}
           });

            // add logged in request
            if (!_sessionToken.empty())
                sess.UpdateHeader({
                                          {"X-Parse-Session-Token", _sessionToken}
                                  });


            return std::move(sess);
        }
    };

    StormIR::StormIR(std::string appid, std::string clientkey) {
        _impl = std::make_unique<StormIRImpl>();

        _impl->appid = appid;
        _impl->clientkey = clientkey;
    }

    StormIR::~StormIR() = default;

    bool StormIR::Login(std::string username, std::string password) {
        if (IsConnected())
            return true;

        auto s = _impl->ApiRequest("login");
        s.SetParameters({
                {"username", username},
                {"password", password}
        });

        auto r = s.Get();
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

    bool StormIR::IsConnected() {
        return _impl->_sessionToken != "";
    }

    bool StormIR::SubmitScore(
            const rd::Song *song,
            const rd::Difficulty *diff,
            const Replay &replay,
            const rd::ScoreKeeper &score
    ) {
        if (!IsConnected())
            return false;

        // most of the hard work is done in this function
        auto j = SerializeScore(
                song,
                diff,
                replay.GetDifficultyIndex(),
                score,
                replay.GetEffectiveParameters()
        );
        auto s = _impl->ApiRequest("functions/submitScore");
        auto b = j.dump();
        s.SetBody(b);

        auto r = s.Post();
        if (r.error) {
            return false;
        }

        return true;
    }

    void StormIR::FetchPersonalScore() {

    }

    std::string StormIR::GetSessionToken() {
        return _impl->_sessionToken;
    }

    std::string StormIR::GetLastError() {
        return last_error;
    }
}