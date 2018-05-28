#include "pch.h"
#include "ext/unzip.h"

#include "Song7K.h"
#include "NoteLoader7K.h"

#include "Audiofile.h"
#include "AudioSourceMP3.h"

using namespace Game::VSRG;

/*
    This is pretty much the simplest possible loader.

	It's important to consider the most complex part is reading from the zip files.

    The important part is: all notes should contain a start time, possibly an end time (both in seconds)
    and you require at least a single BPM to calculate speed properly. And that's it, set the duration
    of the chart and the loading is done.
*/

void NoteLoaderFTB::LoadMetadata(std::string filename, std::string prefix, Song *Out)
{
#if (!defined _WIN32) || (defined STLP)
    std::ifstream filein(filename.c_str());
#else
    std::ifstream filein(Utility::Widen(filename).c_str());
#endif

    if (!filein.is_open())
        return;

    std::string Author;
    std::string Title;
    std::string musName;

    getline(filein, musName);
    getline(filein, Title);
    getline(filein, Author);

    Out->SongFilename = musName;
    Out->Title = Title;
    Out->Artist = Author;
    Out->SongDirectory = prefix + "/";

    filein.close();
}

void LoadFTBFromString(std::string s, Difficulty *Diff)
{
	std::stringstream filein;
	filein << s;
	Measure Msr;

	int cnt = 0;

	while (filein)
	{
		std::string Line;
		std::getline(filein, Line);

		Line = Utility::Trim(Line);

		if (Line[0] == '#' || Line.length() == 0)
			continue;

		auto LineContents = Utility::TokenSplit(Line, " ");

		if (LineContents.at(0) == "BPM")
		{
			TimingSegment Seg;
			Seg.Time = latof(LineContents[1].c_str()) / 1000.0;
			Seg.Value = latof(LineContents[2].c_str());
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
				Note.StartTime = latof(NoteInfo.at(0).c_str()) / 1000.0;
				Note.EndTime = latof(NoteInfo.at(1).c_str()) / 1000.0;
			}
			else
			{
				Note.StartTime = latof(NoteInfo.at(0).c_str()) / 1000.0;
			}

			/* index 1 is unused */
			int Track = atoi(LineContents[2].c_str()); // Always > 1

			Diff->Duration = std::max(std::max(Note.StartTime, Note.EndTime), Diff->Duration);
			Msr.Notes[Track - 1].push_back(Note);
			cnt++;
		}
	}

	// Offsetize
	if (Diff->Timing.size())
	{
		Diff->Offset = Diff->Timing.begin()->Time;

		for (auto i : Diff->Timing)
			i.Time -= Diff->Offset;
	}

	Diff->Data->Measures.push_back(Msr);
	Diff->Level = (float)cnt / Diff->Duration;
}

void SetMetadataFromMP3(Song* song)
{
	AudioSourceMP3 source;
	if (source.Open(song->SongFilename)) {
		auto meta = source.GetMetadata();
		song->Artist = meta.artist;
		song->Title = meta.title;
	}

}

void NoteLoaderFTB::LoadObjectsFromFile(std::filesystem::path filename, Song *Out)
{
	auto input = unzOpen(filename.string().c_str());

    if (!input)
    {
failed:
        return;
    }

	// whatever__stuff.ext -> whatever
	auto filestr = filename.replace_extension("").string();
	auto songname = filestr.substr(0, filestr.find_first_of("_"));

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
					std::shared_ptr<Difficulty> Diff(new Difficulty());

					// read file
					auto len = info.uncompressed_size;
					char* data = new char[len+1];
					unzReadCurrentFile(input, data, len);

					// read file into difficulty
					Diff->Filename = filename.replace_extension("ft2");

					Diff->BPMType = Difficulty::BT_MS; // MS using BPMs.
					Diff->Channels = 7;
					Diff->Name = name;
					

					std::shared_ptr<DifficultyLoadInfo> LInfo(new DifficultyLoadInfo());
					Diff->Data = LInfo;
					Diff->Data->TimingInfo = std::make_shared<StepmaniaChartInfo>();

					LoadFTBFromString(std::string(data, len), Diff.get());

					delete[] data;
					unzCloseCurrentFile(input);

					// done!
					Out->Difficulties.push_back(Diff);
				} // opened file
			}
			else continue; // no file info
		} while (unzGoToNextFile(input) == UNZ_OK);
	}

	unzClose(input);

	// done with difficulties. Now metadata
	Out->SongFilename = filestr + ".ftb";
	SetMetadataFromMP3(Out);
}
