#pragma once

namespace Game {
	namespace VSRG {
		namespace NoteTransform
		{
			void Randomize(VectorTN &Notes, int ChannelCount, bool RespectScratch = false);
			void Mirror(VectorTN &Notes, int ChannelCount, bool RespectScratch = false);
			void MoveKeysoundsToBGM(unsigned char channels, VectorTN notes_by_channel, std::vector<AutoplaySound> &bg_ms, double drift);
			void TransformToBeats(unsigned char channels, VectorTN notes_by_channel, const TimingData &BPS);
		}
	}
}
