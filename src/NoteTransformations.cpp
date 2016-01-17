#include <random>
#include <ctime>

#include "GameGlobal.h"
#include "Song7K.h"

namespace NoteTransform {
	void Randomize(VSRG::VectorTN &Notes, int ChannelCount, bool RespectScratch)
	{
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
	}

	void Mirror(VSRG::VectorTN &Notes, int ChannelCount, bool RespectScratch)
	{
		int k;
		if (RespectScratch) k = 1; else k = 0;
		for (int v = ChannelCount - 1 ; k < ChannelCount / 2; k++, v--)
			swap(Notes[k], Notes[v]);
	}

	void MoveKeysoundsToBGM(unsigned char channels, VSRG::VectorTN notes_by_channel, vector<AutoplaySound> &bg_ms)
	{
		for (int k = 0; k < channels; k++)
		{
			for (auto&& n : notes_by_channel[k])
			{
				bg_ms.push_back(AutoplaySound { float(n.GetStartTime()), n.GetSound() });
				n.RemoveSound();
			}
		}
	}
}