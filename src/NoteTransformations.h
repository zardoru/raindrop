#pragma once

namespace NoteTransform{
	void Randomize(VSRG::VectorTN &Notes, int ChannelCount, bool RespectScratch = false);
	void Mirror(VSRG::VectorTN &Notes, int ChannelCount, bool RespectScratch = false);
}
