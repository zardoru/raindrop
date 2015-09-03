#include <random>
#include <ctime>

#include "GameGlobal.h"
#include "Song7K.h"

namespace NoteTransform {
	void Randomize(VSRG::VectorTN &Notes, int ChannelCount, bool RespectScratch)
	{
		vector<int> chan_index;
		int si;

		if (RespectScratch)
			si = 1; else si = 0;

		for (auto k = si; k < ChannelCount; k++)
		{
			chan_index.push_back(k);
		}

		std::mt19937 dev;
		dev.seed(time(nullptr));
		std::uniform_int_distribution<int> di(si, ChannelCount - 1);

		std::random_shuffle(chan_index.begin() + si, chan_index.end(), [&](int i) -> int { return di(dev); });

		for (auto k = si; k < ChannelCount; k++)
			swap(Notes[k], Notes[chan_index[k]]);
	}

	void Mirror(VSRG::VectorTN &Notes, int ChannelCount, bool RespectScratch)
	{
		int k;
		if (RespectScratch) k = 1; else k = 0;
		for (; k < ChannelCount; k++)
			swap(Notes[k], Notes[ChannelCount - 1]);
	}

}