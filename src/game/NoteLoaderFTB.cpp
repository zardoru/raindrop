#include <filesystem>
#include <rmath.h>
#include <minizip/unzip.h>

#include <future>
#include <sndio/Audiofile.h>
#include <sndio/AudioSourceMP3.h>

#include <fstream>
#include <game/Song.h>
#include <game/NoteLoader7K.h>
#include "TextAndFileUtil.h"

using namespace rd;

/*
    This is pretty much the simplest possible loader.

	It's important to consider the most complex part is reading from the zip files.

    The important part is: all notes should contain a start time, possibly an end time (both in seconds)
    and you require at least a single BPM to calculate speed properly. And that's it, set the duration
    of the chart and the loading is done.
*/
void LoadFTBFromString(std::string s, rd::Difficulty *Diff)
{
	Measure Msr;

	int cnt = 0;

	auto lines = Utility::TokenSplit(s, "\r\n", true);
	for(auto Line : lines)
	{
		Line = Utility::Trim(Line);

		if (Line[0] == '#' || Line.length() == 0)
			continue;

		auto LineContents = Utility::TokenSplit(Line, " ");

		if (LineContents.at(0) == "BPM")
		{
			TimingSegment Seg;
			Seg.Time = latof(LineContents[1]) / 1000.0;
			Seg.Value = latof(LineContents[2]);
			Diff->Timing.push_back(Seg);
		}
		else
		{
			/* We'll make a few assumptions about the structure from now on
			->The vertical speeds determine note position, not measure
			->More than one measure is unnecessary when using BT_MS,
			so we'll use a single measure for all the song, containing all notes.
			*/

			NoteData Note;
			auto NoteInfo = Utility::TokenSplit(LineContents.at(0), "-");
			if (NoteInfo.size() > 1)
			{
				Note.StartTime = latof(NoteInfo.at(0)) / 1000.0;
				Note.EndTime = latof(NoteInfo.at(1)) / 1000.0;
			}
			else
			{
				Note.StartTime = latof(NoteInfo.at(0)) / 1000.0;
			}

			/* index 1 is unused */
			int Track = atoi(LineContents[2].c_str()); // Always > 1

			Diff->Duration = std::max(std::max(Note.StartTime, Note.EndTime), Diff->Duration);
			
			if (Track > 0)
				Msr.Notes[Track - 1].push_back(Note);

			cnt++;
		}
	}

	/*for (auto &lane : Msr.Notes) { // Sort lanes by time.
		std::sort(lane.begin(), lane.end(), [](const NoteData &A, const NoteData &B) {
			return A.StartTime < B.StartTime;
		});
	}*/

	// Offsetize
	if (Diff->Timing.size() == 0)
	{
		Diff->Timing.push_back(TimingSegment{ 0, 120 });
	}

	/*Diff->Offset = Diff->Timing[0].Time;

	for (auto &i : Diff->Timing)
		i.Time -= Diff->Offset;*/

	Diff->Data->Measures.push_back(Msr);
	Diff->Level = (float)cnt / Diff->Duration;
}

void SetMetadataFromMP3(rd::Song* song)
{
	AudioSourceMP3 source;
	if (source.Open(song->SongDirectory / song->SongFilename)) {
		auto meta = source.GetMetadata();
		song->Artist = Utility::Trim(meta.artist);
		song->Title = Utility::Trim(meta.title);

		//if (song->Artist == "" || song->Title == "") {
		//	Log::LogPrintf("%s is missing metadata information\n", song->SongFilename.string().c_str());
		//}

		double lenSecs = ((double)source.GetLength()) / source.GetRate();
		for (auto & diff : song->Difficulties) {
			diff->Duration = std::min(diff->Duration, lenSecs);

			// fakeify
			for (auto &msr : diff->Data->Measures) {
				for (int i = 0; i < 7; i++) {
					for (auto &note : msr.Notes[i]) {
						if (note.StartTime > lenSecs) {
							note.NoteKind = rd::NK_FAKE;
						}
					}
				}
			}

		}
	}

}

void NoteLoaderFTB::LoadObjectsFromFile(const std::filesystem::path& filename, rd::Song *Out)
{
	auto input = unzOpen(filename.string().c_str());

    if (!input)
    {
        return;
    }

	// whatever__stuff.ext -> whatever
	auto filestr = filename.filename().replace_extension("").string();
	auto songname = filestr.substr(0, filestr.find_last_of("_"));

	if (unzGoToFirstFile(input) == UNZ_OK) {
		do {
			unz_file_info info;
			char full_filename[1024];

			if (unzGetCurrentFileInfo(
				input,
				&info,
				full_filename,
				1024,
				NULL, 0, NULL, 0) == UNZ_OK) {
				auto name = std::string(full_filename, strchr(full_filename, '.'));
				if (unzOpenCurrentFile(input) == UNZ_OK) {
					auto Diff = std::make_unique<rd::Difficulty>();

					// read file
					auto len = info.uncompressed_size;
					std::vector<uint8_t> data(len+1);
					unzReadCurrentFile(input, data.data(), len);

					// read file into difficulty
					Diff->Filename = filename;
					Diff->Filename = Diff->Filename.replace_extension("ft2");

					Diff->BPMType = rd::Difficulty::BT_MS; // MS using BPMs.
					Diff->Channels = 7;
					Diff->Name = name;
					

					Diff->Data = std::make_unique<rd::DifficultyLoadInfo>();
					Diff->Data->TimingInfo = std::make_unique<rd::StepmaniaChartInfo>();

					std::string ftb_data((const char*)data.data(), len);

					LoadFTBFromString(ftb_data, Diff.get());

					// done!
					if (Diff->Duration > 0)
						Out->Difficulties.push_back(std::move(Diff));

					unzCloseCurrentFile(input);
				} // opened file
			}
			else continue; // no file info
		} while (unzGoToNextFile(input) == UNZ_OK);
	}

	unzClose(input);

	// done with difficulties. Now metadata
	Out->SongFilename = filename.filename().replace_extension("ftb");
	Out->SongPreviewSource = Out->SongFilename;
	SetMetadataFromMP3(Out);
	if (Out->Title.empty()) {
		Out->Title = songname;
	}
}