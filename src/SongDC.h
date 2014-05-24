#include "Song.h"
#include "GameObject.h"

namespace dotcur {

	typedef std::vector<GameObject> Measure;


	struct Difficulty : public Game::Song::Difficulty
	{
		// Stores bpm at beat pairs
		TimingData Timing;

		// Notes
		std::vector<Measure> Measures;

		// Stores the ratio barline should move at a certain time
		TimingData BarlineRatios;

		std::vector<String> SoundList;
	};

	/* Dotcur Song */
	class Song : public Game::Song
	{
	public:
		Song();
		~Song();

		std::vector<dotcur::Difficulty*> Difficulties;

		/* chart filename*/
		String ChartFilename;

		double		LeadInTime;
		int			MeasureLength;

		void Process(bool CalculateXPos = true);
		void Repack();
		bool Save(const char* Filename);
	};

}