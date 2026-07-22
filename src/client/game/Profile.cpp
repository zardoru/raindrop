#include <queue>
#include <ChartGroup.h>
#include <game/ScoreKeeper.h>


#include <text_and_file_util.h>

#include "PlayscreenParameters.h"
#include "Replay.h"

#include "ScoreDatabase.h"
#include "Profile.h"


const std::filesystem::path PROFILE_DIR = "./profiles";

const std::filesystem::path SCOREDB_FILENAME = "scores.db";

const std::filesystem::path REPLAYS_DIR = "replays";

void Profile::AssureProfilePathExistence() {
    auto path = get_path();
    std::filesystem::create_directories(path);

    auto replaypath = path / REPLAYS_DIR;
    std::filesystem::create_directory(replaypath);
}

std::filesystem::path Profile::get_path() {
    return std::filesystem::absolute(PROFILE_DIR) / name;
}

bool Profile::load(std::string Name) {
    this->name = Name;
    AssureProfilePathExistence();

    auto path = get_path();
    scores.Open(path / SCOREDB_FILENAME);

    return true;
}

bool Profile::save() {
    AssureProfilePathExistence();


    return false;
}

void Profile::save_replay(const otoworm::ChartGroup *chart_group, const Replay &replay) {
    time_t now;
    time(&now);
    auto tm = localtime(&now);

    // format current datetime
    char date_str[512];
    strftime(date_str, 512, "%F %H.%M", tm);

    wchar_t replay_filename[512];

    // format that filename
    swprintf(
            replay_filename,
            512,
            L"[%s] %ls - %ls.rdr",
            otoworm::locale::widen(date_str).c_str(),
            otoworm::locale::widen(chart_group->artist).c_str(),
            otoworm::locale::widen(chart_group->title).c_str()
    );

    // put it out
    replay.save(get_path() / REPLAYS_DIR / replay_filename);
}

std::vector<std::string> Profile::get_profile_list() {
    return std::vector<std::string>();
}
