#include "pch.h"

#include "GameGlobal.h"
#include "Song7K.h"

namespace NoteTransform
{
	void Randomize(VSRG::VectorTN &Notes, int ChannelCount, bool RespectScratch)
	{
		std::vector<int> s;

		// perform action to channel index minus scratch ones if applicable
		auto toChannels = [&](std::function<void(int)> fn) {
			int index = 0;
			for (auto i = 0; i < ChannelCount; i++)
			{
				if (i == 0 && RespectScratch)
				{
					if (ChannelCount == 6 || ChannelCount == 8) {
						index++;
						continue;
					}
				}

				if (i == 5 && ChannelCount == 12 && RespectScratch)
				{
					index++;
					continue;
				}

				if (i == 8 && ChannelCount == 16 && RespectScratch)
				{
					index++;
					continue;
				}

				fn(index);
				index++;
			}
		};


		// get indices of all applicable channels
		toChannels([&](int index)
		{
			s.push_back(index);
		});

		std::mt19937 mt(time(0));
		std::uniform_int_distribution<int> dev;

		// perform random
		// note: random_shuffle's limitation of only swapping with earlier entries
		// makes this preferable.
		for (auto i = 0; i < s.size(); i++)
		{
			std::swap(s[i], s[dev(mt) % s.size()]);
		}

		int limit = int(ceil(s.size() / 2.0));
		int v = 0;
		// to all applicable channels swap with applicable channels
		toChannels([&](int index)
		{
			if (v <= limit) { // avoid cycles
				swap(Notes[index], Notes[s[v]]);
				v++;
			}
		});
    }

    void Mirror(VSRG::VectorTN &Notes, int ChannelCount, bool RespectScratch)
    {
        int k;
        if (RespectScratch) k = 1; else k = 0;
        for (int v = ChannelCount - 1; k < ChannelCount / 2; k++, v--)
            swap(Notes[k], Notes[v]);
    }

    void MoveKeysoundsToBGM(unsigned char channels, VSRG::VectorTN notes_by_channel, std::vector<AutoplaySound> &bg_ms, double drift)
    {
        for (auto k = 0; k < channels; k++)
        {
            for (auto&& n : notes_by_channel[k])
            {
                bg_ms.push_back(AutoplaySound{ float(double(n.GetStartTime()) - drift), n.GetSound() });
                n.RemoveSound();
            }
        }
    }
}