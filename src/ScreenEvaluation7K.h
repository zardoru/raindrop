#pragma once

#include "Screen.h"

class BitmapFont;
class ScoreKeeper;
class SceneEnvironment;

namespace Game {
	namespace VSRG {
		class ScreenGameplay;
		class ScreenEvaluation : public Screen
		{
		public:
			ScreenEvaluation();
			void Init(ScreenGameplay* rs);
			bool Run(double Delta);
			void Cleanup();
			bool HandleInput(int32_t key, KeyEventType code, bool isMouseInput);
			void PrintCLIResults(ScoreKeeper *result);
		};
	}
}