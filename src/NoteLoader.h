/* description: .dcf (dotcur format) reader*/

#ifndef NOTELOADER_H_
#define NOTELOADER_H_

#include <vector>
#include "GameObject.h"
#include "Song.h"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/foreach.hpp>

namespace NoteLoader 
{
	template <class T>
	void LoadBPMs(TSong<T> *Out, SongInternal::TDifficulty<T>* Difficulty, String line)
	{
		String ListString = line.substr(line.find_first_of(":") + 1);
		std::vector< String > SplitResult;
		SongInternal::TDifficulty<T>::TimingSegment Segment;

		// Remove whitespace.
		boost::replace_all(ListString, "\n", "");
		boost::split(SplitResult, ListString, boost::is_any_of(",")); // Separate List of BPMs.
		BOOST_FOREACH(String BPMString, SplitResult)
		{
			std::vector< String > SplitResultPair;
			boost::split(SplitResultPair, BPMString, boost::is_any_of("=")); // Separate Time=Value pairs.

			if (SplitResultPair.size() == 1) // Assume only one BPM on the whole list.
			{
				Segment.Value = atof(SplitResultPair[0].c_str());
				Segment.Time = 0;
			}else // Multiple BPMs.
			{
				Segment.Time = atof(SplitResultPair[0].c_str());
				Segment.Value = atof(SplitResultPair[1].c_str());
			}

			Difficulty->Timing.push_back(Segment);
		}
	}

	// user responsability to clean this one up.
	SongDC *LoadObjectsFromFile(String filename, String prefix = "");
};

#endif // NOTELOADER_H_
