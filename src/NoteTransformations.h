#pragma once

namespace NoteTransform{
	void Randomize(VSRG::VectorTN &Notes, int ChannelCount, bool RespectScratch = false);
	void Mirror(VSRG::VectorTN &Notes, int ChannelCount, bool RespectScratch = false);
	void MoveKeysoundsToBGM(unsigned char channels, VSRG::VectorTN notes_by_channel, vector<AutoplaySound> bg_ms);
}
