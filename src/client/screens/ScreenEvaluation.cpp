#include <memory>

#include <filesystem>
#include <rmath.h>
#include <queue>
#include <future>

#include <Audio.h>
#include <sndio/Audiofile.h>
#include <sndio/AudioSourceOJM.h>

#include "../game/PlayscreenParameters.h"
#include "../game/GameState.h"
#include "../structure/Screen.h"

#include "Transformation.h"
#include "Rendering.h"
#include "Sprite.h"

#include "ScreenEvaluation.h"

#include "../bga/BackgroundAnimation.h"
#include "ScreenGameplay.h"

#include "TextureCollection.h"
#include "../structure/SceneEnvironment.h"

#include "LuaManager.h"
#include "../game/Game.h"


ScreenEvaluation::ScreenEvaluation(GameWindow& window) :
        Screen(window, "ScreenEvaluation7K", false) {
    is_active_ = true;
}

void ScreenEvaluation::init(ScreenGameplay *pr) {
    pr->setup_scripts(scene_->get_script_manager());
    scene_->initialize(GameState::get_instance().get_skin_file("screenevaluation7k.lua"));

    intro_duration_ = scene_->get_intro_duration();
    exit_duration_ = scene_->get_exit_duration();

    change_state(StateIntro);

    // PrintCLIResults(Result);
}

bool ScreenEvaluation::on_input(int32_t key, bool isPressed, bool isMouseInput) {
    auto k = BindingsManager::translate_key(key);
    if ((k == KT_Escape || k == KT_Select) && isPressed)
        is_active_ = false;

    return true;
}

void ScreenEvaluation::cleanup() {
}

bool ScreenEvaluation::run(double Delta) {
    scene_->draw_targets(Delta);
    return is_active_;
}
/*
void ScreenEvaluation::PrintCLIResults(ScoreKeeper *result){
    std::stringstream ss;

    ss
        << "===================\n"
        << "===== RESULTS =====\n"
        << "===================\n"
    ;

    ss << std::fixed << std::setprecision(3);

    if(result->getRank() > 10){
        ss << "  Rank: " << "*" << result->getRank() - 10 << " (" << result->getPercentScore(PST_RANK) << " pts.)\n";
    }else if(result->getRank() > 0){
        ss << "  Rank: " << "+" << result->getRank() << " (" << result->getPercentScore(PST_RANK) << " pts.)\n";
    }else{
        ss << "  Rank: " << result->getRank() << " (" << result->getPercentScore(PST_RANK) << " pts.)\n";
    }

    ss
        << "\n" << std::setprecision(2)
        << "  Accuracy: " << result->getPercentScore(PST_ACC) << "%\n"
        << "  Final Score: " << result->getScore(ST_EXP3) << "\n"
        << "  Max Combo: " << result->getScore(ST_MAX_COMBO) << "\n"
        << "\n"
        << "  Notes hit: " << result->getPercentScore(PST_NH) << "%\n"
        << "  EX score: " << result->getPercentScore(PST_EX) << "%\n"
    ;

    if(result->usesW0()){
        ss << "  osu!mania accuracy: " << result->getPercentScore(PST_OSU) << "%\n";
        ss << "  osu!mania score: " << result->getScore(ST_OSUMANIA) << "\n";
    }

    ss << "\n";

    ss
        << "===== Judgments =====\n"
    ;

    if(result->usesW0())
        ss << " Fantastic: " << result->getJudgmentCount(SKJ_W0) << " (" << float(result->getJudgmentCount(SKJ_W0) * 100) / float(result->getMaxNotes()) << "%)\n";

    ss
        << "   J_PERFECT: " << result->getJudgmentCount(SKJ_W1) << " (" << float(result->getJudgmentCount(SKJ_W1) * 100) / float(result->getMaxNotes()) << "%)\n"
        << "     J_GREAT: " << result->getJudgmentCount(SKJ_W2) << " (" << float(result->getJudgmentCount(SKJ_W2) * 100) / float(result->getMaxNotes()) << "%)\n"
        << "      Good: " << result->getJudgmentCount(SKJ_W3) << " (" << float(result->getJudgmentCount(SKJ_W3) * 100) / float(result->getMaxNotes()) << "%)\n"
        << "       Bad: " << result->getJudgmentCount(SKJ_W4) << " (" << float(result->getJudgmentCount(SKJ_W4) * 100) / float(result->getMaxNotes()) << "%)\n"
        << "        NG: " << result->getJudgmentCount(SKJ_MISS) << " (" << float(result->getJudgmentCount(SKJ_MISS) * 100) / float(result->getMaxJudgableNotes()) << "%)\n"
        << "\n"
    ;

    std::cerr << ss.str();

    //std::cerr << "Histogram:\n" << std::endl;
    //std::cerr << result->getHistogram();
}
*/

