#include "pch.h"


#include "Logging.h"
#include "Song7K.h"
#include "NoteLoader7K.h"

/* osu!mania loader. credits to wanwan159, woc2006, Zorori and the author of AIBat for helping me understand this. */

typedef std::vector<std::string> SplitResult;

using namespace Game::VSRG;

const auto SAMPLESET_NORMAL = 1;
const auto SAMPLESET_SOFT = 2;
const auto SAMPLESET_DRUM = 3;

const auto HITSOUND_NORMAL = 0;
const auto HITSOUND_WHISTLE = 2;
const auto HITSOUND_FINISH = 4;
const auto HITSOUND_CLAP = 8;

const auto NOTE_SLIDER = 2;
const auto NOTE_HOLD = 128;
const auto NOTE_NORMAL = 1;

const double LINE_REMOVE_THRESHOLD = 0.0012;

CfgVar DebugOsuLoader("OsuLoader", "Debug");

std::string GetSamplesetStr(int Sampleset)
{
	switch (Sampleset)
	{
	case SAMPLESET_SOFT:
		return "soft";
	case SAMPLESET_DRUM:
		return "drum";
	case SAMPLESET_NORMAL:
	default:
		return "normal";
	}	
}


namespace Game {
	namespace VSRG {
		TimingData GetBPSData(Difficulty *difficulty, double Drift);
	}
}

enum OsuLoaderReadingState
{
    RNotKnown,
    RGeneral,
    RMetadata,
    RDifficulty,
    REvents,
    RTiming,
    RHitobjects
};

void SetReadingMode(std::string& Line, OsuLoaderReadingState& ReadingMode)
{
	struct
	{
		std::string cnt;
		OsuLoaderReadingState val;
	} state_table[] = {
		{"[General]", RGeneral},
		{"[Metadata]", RMetadata},
		{"[Difficulty]", RDifficulty},
		{"[Events]", REvents},
		{"[TimingPoints]", RTiming},
		{"[HitObjects]", RHitobjects},
	};

	for (auto x: state_table)
	{
		if (x.cnt == Line) { ReadingMode = x.val; return; }
	}

	if (Line.find_first_of("[") == 0)
		ReadingMode = RNotKnown;
}

struct HitsoundSectionData
{
    int Sampleset;
    int Volume; // In %
    int Custom;
    int IsInherited;
    double Time; // In Seconds
    double Value; // BPM or Multiplier
    double MeasureLen; // In quarter notes
    // bool Kiai;
    bool Omit;
};

bool operator <(const HitsoundSectionData& lhs, const HitsoundSectionData& rhs)
{
    return lhs.Time < rhs.Time;
}

bool operator< (const HitsoundSectionData& lhs, const double rhs) {
	return lhs.Time < rhs;
}

int GetTrackFromPosition(float Position, int Channels)
{
    float Step = 512.f / Channels;

    return static_cast<int>(Position / Step);
}

class OsuManiaLoaderException : public std::exception
{
private:
	std::string msg;
public:
	OsuManiaLoaderException(const char * what) : exception(), msg(what) {}
	const char* what() const noexcept { return msg.c_str(); }
};

class OsumaniaLoader
{
    double slider_velocity;
    int Version;
    int last_sound_index;
    Song *osu_sng;
    std::shared_ptr<OsumaniaChartInfo> TimingInfo;
    std::map <std::string, int> Sounds;
    std::vector<HitsoundSectionData> HitsoundSections;
    std::shared_ptr<Difficulty> Diff;
    std::string DefaultSampleset;

	std::stringstream EventsContent;

    bool ReadAModeTag;

    std::vector<NoteData> Notes[MAX_CHANNELS];
    int line_number;

	void Offsetize()
	{
		auto first = FindFirstMeasure();
		Diff->Offset = first->Time;

		for (auto i = HitsoundSections.begin();
		i != HitsoundSections.end();
			++i)
		{
			i->Time -= Diff->Offset;
		}
	}

	std::vector<HitsoundSectionData>::iterator FindFirstMeasure()
	{
		auto i = HitsoundSections.begin();
		auto next = i + 1;
		while (i != HitsoundSections.end() && i->IsInherited)
		{
			++i;
			next = i + 1;
			if (i != HitsoundSections.end() && next != HitsoundSections.end())
			{
				if (!i->IsInherited && !next->IsInherited)
				{
					while (i->Time - next->Time < 0.001 &&
							i != HitsoundSections.end() &&
							next != HitsoundSections.end())
					{
						i++;
						next = i + 1;
					}
				}
			}
		}

		return i != HitsoundSections.end() ? i : HitsoundSections.begin();
	}

	void MeasurizeFromTimingData()
	{
		Offsetize();

		auto seclst = filter([](const HitsoundSectionData& H) {return !H.IsInherited && !H.Omit;}, HitsoundSections);
		for (auto i = seclst.begin(); i != seclst.end();)
		{
			double SectionDurationInBeats = 0;
			double acom_mlen = i->MeasureLen;

			/* 
			
			if there's an older one that is 1ms later
			 we've got to skip to it osu doesn't display the current one
			it just displays the last one
			thanks nivrad00 for finding this glitch
			
			Emulating the skip bug implies we
			skip all of the following lines 
			up to the next one 
			that does not have a line with a line before it 
			with a time distance less than 1ms. 

			The most important detail about it is that 
			the displacement of the measure line 
			is actually applied
			it's merely not shown
			*/
			auto next = i + 1;
			if (next != seclst.end())
			{
				auto bt = (next->Time - i->Time) * bps(i->Value);
				SectionDurationInBeats += bt;

				if (DebugOsuLoader)
					Log::LogPrintf("\nMCALC: %f to %f (%f beats long at %f bpm)", next->Time, i->Time, bt, i->Value);

				// the next section, and the one after that
				auto c_next = next;
				auto sk_next = next + 1;

				// add nivrad's bug
				do {
					// and that next line has a time distance of less than the threshold
					if (sk_next != seclst.end() && sk_next->Time - c_next->Time <= LINE_REMOVE_THRESHOLD) 
					{
						// ah the one after and the next are less than 1ms apart
						// get the displacement in beats of this section
						auto seg = (sk_next->Time - c_next->Time) * bps(c_next->Value);
						if (DebugOsuLoader)
							Log::LogPrintf("\nMCALC: %f to %f (section is %g beats long)", sk_next->Time, c_next->Time, seg);
						SectionDurationInBeats += seg; // we displace along here
						++c_next;
						++sk_next;
					}
					else {
						break; // otherwise we stop looking for the next line
					}
				} while (true);

				// either what we skipped or the inmediate next
				// if what we skipped we're out of the 1ms woods
				// anyway, the one after lasts more than 1ms
				next = c_next;
			} else
			{
				auto seg = bps(i->Value) * (Diff->Duration - i->Time);
				SectionDurationInBeats += seg;
				if (DebugOsuLoader)
					Log::LogPrintf("\nMCALC: Last seg. %f to %f (%g beats long)", Diff->Duration, i->Time, seg);

			}
			
			auto TotalMeasuresThisSection = SectionDurationInBeats / i->MeasureLen;
			if (DebugOsuLoader)
				Log::LogPrintf("\nTotal Measures: %g, beats: %g, mlen: %f", TotalMeasuresThisSection, SectionDurationInBeats, i->MeasureLen);


			auto Whole = floor(TotalMeasuresThisSection);
			auto Fraction = TotalMeasuresThisSection - Whole;

			// Add the measures.
			for (auto k = 0; k < Whole; k++)
			{
				Measure Msr;
				Msr.Length = i->MeasureLen;
				Diff->Data->Measures.push_back(Msr);
			}

			if (Fraction > 0)
			{
				auto dur_secs = Fraction * i->MeasureLen * spb(i->Value);
				if (dur_secs > LINE_REMOVE_THRESHOLD) {
					Measure Msr;
					Msr.Length = Fraction * (i)->MeasureLen;
					Diff->Data->Measures.push_back(Msr);
				}
				else {
					if (Diff->Data->Measures.size())
						Diff->Data->Measures.back().Length += Fraction * i->MeasureLen;
				}
			}

			i = next;
		}
	}

	
	void PushNotesToMeasures()
	{
		TimingData BPS = Game::VSRG::GetBPSData(Diff.get(), 0);

		for (int k = 0; k < MAX_CHANNELS; k++)
		{
			for (auto i = Notes[k].begin(); i != Notes[k].end(); ++i)
			{
				double Beat = QuantizeBeat(IntegrateToTime(BPS, i->StartTime));
				double CurrentBeat = 0; // Lower bound of this measure

				if (Beat < 0)
				{
					Diff->Data->Measures[0].Notes[k].push_back(*i);
					continue;
				}

				for (auto m = Diff->Data->Measures.begin(); m != Diff->Data->Measures.end(); ++m)
				{
					double NextBeat = std::numeric_limits<double>::infinity();
					auto nextm = m + 1;

					if (nextm != Diff->Data->Measures.end()) // Higher bound of this measure
						NextBeat = CurrentBeat + m->Length;

					if (Beat >= CurrentBeat && Beat < NextBeat) // Within bounds
					{
						m->Notes[k].push_back(*i); // Add this note to this measure.
						break; // Stop looking for a measure.
					}

					CurrentBeat += m->Length;
				}
			}
		}
	}

public:
    double GetBeatspaceAt(double T)
    {
        double Ret;
        if (HitsoundSections.size())
        {
            auto Current = HitsoundSections.begin();
            while (Current != HitsoundSections.end() && (Current->Time > T || Current->IsInherited))
                ++Current;

            if (Current == HitsoundSections.end())
            {
                Current = HitsoundSections.begin();
                while (Current != HitsoundSections.end() && Current->IsInherited)
                    ++Current;

                if (Current == HitsoundSections.end())
                    throw OsuManiaLoaderException("No uninherited timing points were found!");
                else
                    Ret = Current->Value;
            }
            else
                Ret = Current->Value;
        }
        else
            throw OsuManiaLoaderException("No timing points found on this osu! file.");

        return Ret;
    }

    OsumaniaLoader(Song *song): slider_velocity(1.4), Version(0), last_sound_index(1), osu_sng(song)
    {
        ReadAModeTag = false;
        line_number = 0;
    }

    double GetSliderMultiplierAt(double T)
    {
		auto seclst = filter([&](const HitsoundSectionData &H) { return H.IsInherited && H.Time >= T; }, HitsoundSections);
        return seclst.size() ? seclst[0].Value : 1;
    }

	bool ReadGeneral(std::string line)
	{
		std::string Command = line.substr(0, line.find_first_of(":") + 1); // Lines are Information:<space>Content
		std::string Content = line.substr(line.find_first_of(":") + 1);

		Utility::Trim(Content);

		if (Command == "AudioFilename:")
		{
			if (Content == "virtual")
			{
				Diff->IsVirtual = true;
				return true;
			}
			else
			{
	#ifdef VERBOSE_DEBUG
				printf("Audio filename found: %s\n", Content.c_str());
	#endif
				Utility::Trim(Content);
				osu_sng->SongFilename = Content;
				osu_sng->SongPreviewSource = Content;
			}
		}
		else if (Command == "Mode:")
		{
			ReadAModeTag = true;
			if (Content != "3") // It's not a osu!mania chart, so we can't use it.
				return false;
		}
		else if (Command == "SampleSet:")
		{
			Utility::ToLower(Content); Utility::Trim(Content);
			DefaultSampleset = Content;
		}
		else if (Command == "PreviewTime:")
		{
			if (Content != "-1")
			{
				if (osu_sng->PreviewTime == 0)
					osu_sng->PreviewTime = latof(Content) / 1000;
			}
		}
		else if (Command == "SpecialStyle:")
		{
			if (Content == "1")
				Diff->Data->Turntable = true;
		}

		return true;
	}


	void ReadMetadata(std::string line)
	{
		auto Command = line.substr(0, line.find_first_of(":")); // Lines are Information:Content
		auto Content = line.substr(line.find_first_of(":") + 1, line.length() - line.find_first_of(":"));

	#ifdef VERBOSE_DEBUG
		printf("Command found: %s | Contents: %s\n", Command.c_str(), Content.c_str());
	#endif

		Utility::Trim(Content);
		if (Command == "Title")
		{
			osu_sng->SongName = Content;
		}
		else if (Command == "Artist")
		{
			osu_sng->SongAuthor = Content;
		}
		else if (Command == "Version")
		{
			Diff->Name = Content;
		}
		else if (Command == "TitleUnicode")
		{
			if (Content.length() > 1)
				osu_sng->SongName = Content;
		}
		else if (Command == "ArtistUnicode")
		{
			if (Content.length() > 1)
				osu_sng->SongAuthor = Content;
		}
		else if (Command == "Creator")
		{
			Diff->Author = Content;
		}
	}


	void ReadDifficulty(std::string line)
	{
		std::string Command = line.substr(0, line.find_first_of(":")); // Lines are Information:Content
		std::string Content = line.substr(line.find_first_of(":") + 1, line.length() - line.find_first_of(":"));
		Utility::Trim(Content);

		// We ignore everything but the key count!
		if (Command == "CircleSize")
		{
			Diff->Channels = atoi(Content.c_str());
		}
		else if (Command == "SliderMultiplier")
		{
			slider_velocity = latof(Content.c_str()) * 100;
		}
		else if (Command == "HPDrainRate")
		{
			TimingInfo->HP = latof(Content.c_str());
		}
		else if (Command == "OverallDifficulty")
		{
			TimingInfo->OD = latof(Content.c_str());
		}
	}

	void ReadEvents(std::string line)
	{
		auto Spl = Utility::TokenSplit(line);

		if (Spl.size() > 1)
		{
			if (Spl[0] == "0" && Spl[1] == "0")
			{
				Utility::ReplaceAll(Spl[2], "\"", "");
				osu_sng->BackgroundFilename = Spl[2];
				Diff->Data->StageFile = Spl[2];
				EventsContent << line << std::endl;
			}
			else if (Spl[0] == "5" || Spl[0] == "Sample")
			{
				Utility::ReplaceAll(Spl[3], "\"", "");

				if (Sounds.find(Spl[3]) == Sounds.end())
				{
					Sounds[Spl[3]] = last_sound_index;
					last_sound_index++;
				}

				double Time = latof(Spl[1].c_str()) / 1000.0;
				int Evt = Sounds[Spl[3]];
				AutoplaySound New;
				New.Time = Time;
				New.Sound = Evt;

				Diff->Data->BGMEvents.push_back(New);

				Diff->Duration = std::max(Diff->Duration, Time);
			} else
			{
				EventsContent << line << std::endl;
			}
		}
	}

	void ReadTiming(std::string line)
	{
		double Value;
		bool IsInherited;
		auto Spl = Utility::TokenSplit(line);

		if (Spl.size() < 2)
			return;

		if (Spl[6] == "1") // Non-inherited section
			IsInherited = false;
		else // An inherited section would be added to a velocity changes vector which would later alter speeds.
			IsInherited = true;

		int Sampleset = -1;
		int Custom = 0;
		double MeasureLen = 4;

		// We already set the value
		if (Spl.size() > 1 && !IsInherited)
			Value = 60000 / latof(Spl[1].c_str());
		else
			Value = -100 / latof(Spl[1].c_str());

		if (Spl.size() > 2)
			MeasureLen = latof(Spl[2].c_str());

		if (Spl.size() > 3)
			Sampleset = atoi(Spl[3].c_str());

		if (Spl.size() > 4)
			Custom = atoi(Spl[4].c_str());

		HitsoundSectionData SecData;
		SecData.Value = Value;
		SecData.MeasureLen = MeasureLen;
		SecData.Time = latof(Spl[0].c_str()) / 1000.0;
		SecData.Sampleset = Sampleset;
		SecData.Custom = Custom;
		SecData.IsInherited = IsInherited;
		SecData.Omit = false; // adjust if taiko bar omission is up

		HitsoundSections.push_back(SecData);
	}
	/*
		This function is mostly correct; the main issue is that we'd have to know
		when custom = 0, that we should use 'per theme' default sounds.
		We don't have those, we don't use those, those are an osu!-ism
		so the sounds are not going to be 100% osu!-correct
		but they're going to be correct enough for virtual-mode charts to be accurate.

		SampleSetAddition is an abomination on a VSRG - so it's only left in for informative purposes.
	*/
	std::string GetSampleFilename(SplitResult &split_line, int NoteType, int Hitsound, double Time)
	{
		// SampleSetAddition is unused but left for self-documenting purposes.
		int SampleSet = 0, SampleSetAddition, CustomSample = 0;
		std::string SampleFilename;

		if (!split_line.size()) // Handle this properly, eventually.
			return "normal-hitnormal.wav";

		auto set_iter = lower_bound(HitsoundSections.begin(), HitsoundSections.end(), Time);
		auto spl_size = split_line.size();

		if (set_iter != HitsoundSections.begin()) --set_iter;

		if (NoteType & NOTE_HOLD)
		{
			if (spl_size > 5 && split_line[5].length())
				return split_line[5];

			if (split_line.size() == 4)
			{
				SampleSet = atoi(split_line[1].c_str());
				SampleSetAddition = atoi(split_line[2].c_str());
				CustomSample = atoi(split_line[3].c_str());
			}
			else
			{
				SampleSet = atoi(split_line[0].c_str());
				SampleSetAddition = atoi(split_line[1].c_str());
				CustomSample = atoi(split_line[2].c_str());
			}

			/*
			if (SplCnt > 4)
				Volume = atoi(Spl[4].c_str()); // ignored lol
			*/
		}
		else if (NoteType & NOTE_NORMAL)
		{
			if (spl_size > 4 && split_line[4].length())
				return split_line[4];

			SampleSet = atoi(split_line[0].c_str());
			if (split_line.size() > 1)
				SampleSetAddition = atoi(split_line[1].c_str());
			if (split_line.size() > 2)
				CustomSample = atoi(split_line[2].c_str());

			/*
			if (SplCnt > 3)
				Volume = atoi(Spl[3].c_str()); // ignored
				*/
		}
		else if (NoteType & NOTE_SLIDER)
		{
			SampleSet = SampleSetAddition = CustomSample = 0;
		}

		std::string set_str;

		if (SampleSet)
		{
			// translate sampleset int into samplesetGString
			set_str = GetSamplesetStr(SampleSet);
		}
		else
		{
			// get sampleset std::string from sampleset active at starttime

			if ((set_iter + 1) == HitsoundSections.begin() || HitsoundSections.begin() == HitsoundSections.end())
				set_str = DefaultSampleset;
			else
				set_str = GetSamplesetStr(set_iter->Sampleset);
		}

		if (!CustomSample && ! (set_iter + 1 == HitsoundSections.begin()) && HitsoundSections.begin() != HitsoundSections.end() )
			CustomSample = set_iter->Custom;

		std::string custom_sample;

		if (CustomSample)
			custom_sample = Utility::IntToStr(CustomSample);

		std::string hitsound_type;

		if (Hitsound)
		{
			switch (Hitsound)
			{
			case 1:
				hitsound_type = "normal";
				break;
			case 2:
				hitsound_type = "whistle";
				break;
			case 4:
				hitsound_type = "finish";
				break;
			case 8:
				hitsound_type = "clap";
			default:
				break;
			}
		}
		else
			hitsound_type = "normal";

		if (CustomSample > 1)
		{
			SampleFilename = set_str + "-hit" + hitsound_type + custom_sample + ".wav";
		}
		else
			SampleFilename = set_str + "-hit" + hitsound_type + ".wav";

		return SampleFilename;
	}

	void ReadObjects(std::string line)
	{
		auto ObjectData = Utility::TokenSplit(line);

		auto Track = GetTrackFromPosition(latof(ObjectData[0].c_str()), Diff->Channels);
		int Hitsound;
		NoteData Note;

		SplitResult ObjectHitsoundData;

		/*
			A few of these "ifs" are just since v11 and v12 store hold endtimes in different locations.
			Or not include some information at all...
		*/
		int splitType = 5;
		if (ObjectData.size() == 7)
			splitType = 6;
		else if (ObjectData.size() == 5)
			splitType = 4;

		if (splitType != 4) // only 5 entries
			ObjectHitsoundData = Utility::TokenSplit(ObjectData[splitType], ":");

		double startTime = latof(ObjectData[2].c_str()) / 1000.0;
		int NoteType = atoi(ObjectData[3].c_str());

		if (NoteType & NOTE_HOLD)
		{
			double endTime;
			if (splitType == 5 && ObjectHitsoundData.size())
				endTime = latof(ObjectHitsoundData[0].c_str()) / 1000.0;
			else if (splitType == 6)
				endTime = latof(ObjectData[5].c_str()) / 1000.0;
			else // what really? a hold that doesn't bother to tell us when it ends?
				endTime = 0;

			Note.StartTime = startTime;
			Note.EndTime = endTime;

			if (startTime > endTime)
			{ // Okay then, we'll transform this into a regular note..
				if (DebugOsuLoader)
					Log::Printf("NoteLoaderOM: object at track %d has startTime > endTime (%f and %f)\n", Track, startTime, endTime);

				Note.EndTime = 0;
			}
		}
		else if (NoteType & NOTE_NORMAL)
		{
			Note.StartTime = startTime;
		}
		else if (NoteType & NOTE_SLIDER)
		{
			// 6=repeats 7=length
			auto sliderRepeats = latof(ObjectData[6].c_str());
			auto sliderLength = latof(ObjectData[7].c_str());

			auto Multiplier = GetSliderMultiplierAt(startTime);

			auto finalSize = sliderLength * sliderRepeats * Multiplier;
			auto beatDuration = (finalSize / slider_velocity);
			auto bpm = (60000.0 / GetBeatspaceAt(startTime));
			auto len_seconds = beatDuration * spb(bpm);

			if (0 > len_seconds && DebugOsuLoader)
				Log::LogPrintf("Line %d: o!m loader warning: object at track %d has startTime > endTime (%f and %f)\n", line_number, Track, startTime, len_seconds + startTime);

			Note.StartTime = startTime;
			Note.EndTime = len_seconds + startTime;
		}

		Hitsound = atoi(ObjectData[4].c_str());

		auto Sample = GetSampleFilename(ObjectHitsoundData, NoteType, Hitsound, startTime);

		if (Sample.length())
		{
			if (Sounds.find(Sample) == Sounds.end())
			{
				Sounds[Sample] = last_sound_index;
				last_sound_index++;
			}

			Note.Sound = Sounds[Sample];
		}

		Notes[Track].push_back(Note);

		Diff->Duration = std::max(std::max(Note.StartTime, Note.EndTime) + 1, Diff->Duration);
	}

	void CopyTimingData()
	{
		for (auto S : HitsoundSections)
		{
			if (S.IsInherited)
				Diff->Data->Scrolls.push_back(TimingSegment(S.Time, S.Value));
			else
				Diff->Timing.push_back(TimingSegment(S.Time, 60000 / S.Value));
		}
	}

	void LoadFromFile(std::filesystem::path path)
    {
		CreateIfstream(filein, path);

		std::regex versionfmt("osu file format v(\\d+)");

		if (!filein.is_open())
			throw OsuManiaLoaderException("Could not open file.");

	    TimingInfo = std::make_shared<OsumaniaChartInfo>();
		Diff = std::make_shared<Difficulty>();
		Diff->Data = std::make_shared<DifficultyLoadInfo>();
		Diff->Data->TimingInfo = TimingInfo;

		// osu! stores bpm information as the time in ms that a beat lasts.
		Diff->BPMType = Difficulty::BT_BEATSPACE;
		Diff->Filename = path;
		
		/*
			Just like BMS, osu!mania charts have timing data separated by files
			and a set is implied using folders.
		*/

		std::string line_str;

		getline(filein, line_str);
		int version = -1;
		std::smatch sm;

		// "search" was picked instead of "match" since a line can have a bunch of
		// junk before the version declaration
		if (regex_search(line_str.cbegin(), line_str.cend(), sm, versionfmt))
			version = atoi(sm[1].str().c_str());
		else
			throw OsuManiaLoaderException("Invalid .osu file.");

		// "osu file format v"
		if (version < 10) // why
			throw OsuManiaLoaderException(Utility::Format("Unsupported osu! file version (%d < 10)", version).c_str());

		Version = version;

		OsuLoaderReadingState ReadingMode = RNotKnown, ReadingModeOld = RNotKnown;

		try
		{
			while (filein)
			{
				line_number++;
				getline(filein, line_str);
				Utility::ReplaceAll(line_str, "\r", "");

				if (!line_str.length())
					continue;

				SetReadingMode(line_str, ReadingMode);

				if (ReadingMode != ReadingModeOld || ReadingMode == RNotKnown) // Skip this line since it changed modes, or it's not a valid section yet
				{
					if (ReadingModeOld == RTiming)
						std::stable_sort(HitsoundSections.begin(), HitsoundSections.end());
					if (ReadingModeOld == RGeneral)
						if (!ReadAModeTag)
							throw OsuManiaLoaderException("Not an osu!mania chart.");

					if (ReadingModeOld == REvents)
						Diff->Data->osbSprites = std::make_shared<osb::SpriteList>(ReadOSBEvents(EventsContent));

					ReadingModeOld = ReadingMode;
					continue;
				}

				switch (ReadingMode)
				{
				case RGeneral:
					if (!ReadGeneral(line_str))  // don't load charts that we can't work with
						throw OsuManiaLoaderException("osu! file unusable on raindrop.");
					break;
				case RMetadata: ReadMetadata(line_str); break;
				case RDifficulty: ReadDifficulty(line_str); break;
				case REvents: ReadEvents(line_str); break;
				case RTiming: ReadTiming(line_str); break;
				case RHitobjects: ReadObjects(line_str); break;
				default: break;
				}
			}

			if (Diff->Data->GetObjectCount())
			{
				// Okay then, convert timing data into a measure-based format raindrop can use 
				// and calculate offset.
				MeasurizeFromTimingData();

				// Move timing data into difficulty.
				CopyTimingData();

				// Then copy notes into these measures.
				PushNotesToMeasures();

				// Copy all sounds we registered
				for (auto i : Sounds)
					Diff->Data->SoundList[i.second] = i.first;

				// Calculate level as NPS
				Diff->Level = Diff->Data->GetScoreItemsCount() / Diff->Duration;
				osu_sng->Difficulties.push_back(Diff);
			}
		}
		catch (std::exception &e)
		{
			// rethrow with line info
			throw OsuManiaLoaderException(Utility::Format("Line %d: %s", line_number, e.what()).c_str());
		}
    }
};


void NoteLoaderOM::LoadObjectsFromFile(std::filesystem::path filename, Song *Out)
{
    OsumaniaLoader Info(Out);

	Info.LoadFromFile(filename);
}
