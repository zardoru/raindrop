#pragma once

#include "Screen.h"

class BitmapFont;

namespace Game {
	namespace dotcur {
		class ScreenEvaluation : public Screen
		{
			EvaluationData Results;
			Sprite Background;
			BitmapFont* Font;

			std::string ResultsGString, ResultsNumerical;
			std::string TitleFormat;

			int32_t CalculateScore();
		public:
			ScreenEvaluation();
			void Init(EvaluationData _Data, std::string SongAuthor, std::string SongTitle);
			bool Run(double Delta) override;
			void Cleanup() override;
			bool HandleInput(int32_t key, KeyEventType code, bool isMouseInput) override;
		};
	}
}