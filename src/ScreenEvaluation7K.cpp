#include "pch.h"


#include "GameState.h"
#include "ScreenEvaluation7K.h"
#include "ScreenGameplay7K.h"
#include "ImageLoader.h"
#include "Audio.h"
#include "ScoreKeeper7K.h"
#include "SceneEnvironment.h"
#include "LuaManager.h"

namespace Game {
	namespace VSRG {

		ScreenEvaluation::ScreenEvaluation() :
			Screen("ScreenEvaluation7K", false)
		{
			Running = true;
		}

		void ScreenEvaluation::Init(ScreenGameplay *pr)
		{
			Animations->InitializeUI();

			pr->SetupLua(Animations->GetEnv());
			Animations->Initialize(GameState::GetInstance().GetSkinFile("screenevaluation7k.lua"));

			IntroDuration = Animations->GetIntroDuration();
			ExitDuration = Animations->GetExitDuration();

			ChangeState(StateIntro);

			// PrintCLIResults(Result);
		}

		bool ScreenEvaluation::HandleInput(int32_t key, KeyEventType code, bool isMouseInput)
		{
			if ((BindingsManager::TranslateKey(key) == KT_Escape || BindingsManager::TranslateKey(key) == KT_Select) && code == KE_PRESS)
				Running = false;

			return true;
		}

		void ScreenEvaluation::Cleanup()
		{
		}

		bool ScreenEvaluation::Run(double Delta)
		{
			Animations->DrawTargets(Delta);
			return Running;
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
				<< "        NG: " << result->getJudgmentCount(SKJ_MISS) << " (" << float(result->getJudgmentCount(SKJ_MISS) * 100) / float(result->getMaxNotes()) << "%)\n"
				<< "\n"
			;

			std::cerr << ss.str();

			//std::cerr << "Histogram:\n" << std::endl;
			//std::cerr << result->getHistogram();
		}
		*/

	}
}