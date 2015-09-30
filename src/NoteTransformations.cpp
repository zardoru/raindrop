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

		std::mt19937 dev;
		dev.seed(time(nullptr));
		std::uniform_int_distribution<int> di(si, ChannelCount - 1);

		std::random_shuffle(Notes + si, Notes + ChannelCount, [&](int i) -> int {
			int rv = -1;
			while (rv < si)
				rv = di(dev) % i; // Only can swap with earlier element, and it's the i-th position, not index, so let's % it.
			return rv;
		});

		for (auto k = si; k < chan_index.size(); k++)
			swap(Notes[k], Notes[chan_index[k-1]]);
	}

	void Mirror(VSRG::VectorTN &Notes, int ChannelCount, bool RespectScratch)
	{
		int k;
		if (RespectScratch) k = 1; else k = 0;
		for (int v = ChannelCount - 1 ; k < ChannelCount / 2; k++, v--)
			swap(Notes[k], Notes[ChannelCount - k]);
	}

}