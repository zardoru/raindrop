#include "Song.h"
#include "GameObject.h"

namespace dotcur {

	typedef std::vector<GameObject> Measure;


	struct Difficulty : public Game::Song::Difficulty
	{
		// Notes
		std::vector<Measure> Measures;

		// Stores the ratio barline should move at a certain time
		TimingData BarlineRatios;
	};

	/* Dotcur Song */
	class Song : public Game::Song
	{
	public:
		Song();
		~Song();

		std::vector<dotcur::Difficulty*> Difficulties;

		/* chart filename*/
		GString ChartFilename;

		double		LeadInTime;
		int			MeasureLength;

		dotcur::Difficulty* GetDifficulty(uint32 i);
		void Process(bool CalculateXPos = true);
		void Repack();
		bool Save(const char* Filename);
	};

}