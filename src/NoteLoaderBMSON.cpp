#include <json/json.h>
#include <fstream>
#include <unordered_set>

#include "GameGlobal.h"
#include "Song7K.h"
#include <regex>
#include "Logging.h"

// All non-standard exceptions are marked with NSE.

namespace NoteLoaderBMS
{
	GString GetSubtitles(GString SLine, std::unordered_set<GString> &Out);
}

namespace NoteLoaderBMSON{

	const char* UNSPECIFIED_VERSION = "0.21";
	const char* VERSION_1 = "1.0.0";

	// Beat = locate / resolution. 
	const double BMSON_DEFAULT_RESOLUTION = 240.0;

	struct BmsonNote
	{
		int Sound; // Slice mapped to.
		double Length; // in beats
	};

	const struct
	{
		int bmson_level;
		int rank_level;
	} level_bindings[] = {
		{ 52, 1 },
		{ 60, 2 },
		{ 70, 3 },
		{ 100, 4 },
		{ 120, 5 }
	};

	struct
	{
		const char* hint;
		int keys;
		vector<int> mappings;
	} BmsonLayouts[] = {
		{ "beat-7k", 8, { 1, 2, 3, 4, 5, 6, 7, 0 } },
		{ "beat-5k", 6, { 1, 2, 3, 4, 5, -1, -1, 0 } },
		{ "beat-10k", 12, { 1, 2, 3, 4, 5, -1, -1, 0, 7, 8, 9, 10, 11, -1, -1, 6 } },
		{ "beat-14k", 16, { 1, 2, 3, 4, 5, 6, 7, 0, 9, 10, 11, 12, 13, 14, 15, 8 } },
		{ "popn-5k", 5, { 0, 1, 2, 3, 4 } },
		{ "popn-9k", 9, { 0, 1, 2, 3, 4, 5, 6, 7, 8 } }
	};

	class BMSONLoader {
		Json::Value root;
		std::ifstream &input;
		VSRG::Song* song;
		shared_ptr<VSRG::Difficulty> Chart;
		shared_ptr<VSRG::BMSTimingInfo> TimingInfo;
		std::unordered_set<GString> subtitles;
		GString version;
		double resolution;

		int current_wav;
		vector<int> mappings;

		map<int, map<double, BmsonNote> > Notes; // int := mapped lane; double := time in beats of obj (for :mix-note)

		SliceContainer Slices;

		GString GetSubartist(const char string[6])
		{
			std::regex sreg(Utility::Format("\\s*%s\\s*:\\s*(.*?)\\s*$", string));
			for (auto s : root["info"]["subartists"])
			{
				std::smatch sm;
				if (regex_search(s.asString(), sm, sreg))
				{
					return sm[1];
				}
			}

			return "";
		}

		void SetChannelsFromModeHint(Json::Value values)
		{
			std::regex generic_keys("generic\\-(\\d+)keys");
			std::regex special_keys("special\\-(\\d+)keys");
			std::smatch sm;

			mappings.clear();

			if (values.isNull())
			{
				// default_layout:
				Chart->Data->Turntable = true;
				Chart->Channels = 8;
				mappings = BmsonLayouts[0].mappings;
				return;
			}

			GString s = values.asCString();
			if (regex_search(s, sm, generic_keys)) {
				int chans = atoi(sm[1].str().c_str());
				if (chans <= VSRG::MAX_CHANNELS)
				{
					Chart->Channels = chans;
					for (int i = 0; i < chans; i++)
						mappings.push_back(i);
					return;
				}
			}

			if (regex_search(s, sm, special_keys)) {
				int chans = atoi(sm[1].str().c_str());
				if (chans <= VSRG::MAX_CHANNELS)
				{
					Chart->Channels = chans;
					Chart->Data->Turntable = true;
					for (int i = 0; i < chans; i++)
						mappings.push_back(i);
					return;
				}
			}

			for (auto layout : BmsonLayouts)
			{
				if (values.asString() == layout.hint)
				{
					Chart->Channels = layout.keys;
					return;
				}
			}

			// Okay then, didn't match anything...
			throw std::exception(Utility::Format("Unknown mode hint: \"%s\"", values.asString().c_str()).c_str());
		}

		void LoadMeta()
		{
			auto meta = root["info"];
			song->SongName = NoteLoaderBMS::GetSubtitles(meta["title"].asString(), subtitles);
			song->SongAuthor = meta["artist"].asString();
			song->Subtitle = Utility::Join(subtitles, " ");

			song->BackgroundFilename = meta["back_image"].asString();

			for (auto &s : meta["subtitles"])
				subtitles.insert(s.asString());

			if (version == UNSPECIFIED_VERSION)
				Chart->Timing.push_back(TimingSegment(0, meta["initBPM"].asDouble()));
			else if (version == VERSION_1)
				Chart->Timing.push_back(TimingSegment(0, meta["init_bpm"].asDouble()));

			Chart->Level = meta["level"].asInt();

			Chart->Author = GetSubartist("chart");

			if (!meta["chart_name"].isNull())
				Chart->Name = meta["chart_name"].asString();

			Chart->BPMType = VSRG::Difficulty::BT_BEAT;

			SetChannelsFromModeHint(meta["mode_hint"]);

			Chart->IsVirtual = true;

			if (!meta["resolution"].isNull())
				resolution = abs(meta["resolution"].asDouble());

			if (resolution == 0) resolution = BMSON_DEFAULT_RESOLUTION; // NSE

			// DEFEXRANK?


			int jRank;
			
			Json::Value jr;
			if (version == UNSPECIFIED_VERSION)
				jr = meta["judgeRank"];
			else
				jr = meta["judge_rank"];

			if (!jr.isNull())
				jRank = jr.asInt();
			else
				jRank = 100;

			for (auto v : level_bindings)
				if (v.bmson_level == jRank)
					TimingInfo->JudgeRank = v.rank_level;
			TimingInfo->GaugeTotal = meta["total"].asInt();
		}

		double FindLastNoteBeat()
		{
			double last_y = -std::numeric_limits<double>::infinity();
			Json::Value sc;
			GetSoundChannels(sc);
			for (auto &&s: sc)
			{
				auto notes = s["notes"];
				for (auto &&note: notes)
				{
					last_y = std::max(note["y"].asDouble(), last_y);
				}
			}

			if (isinf(last_y)) return 0;

			return last_y / resolution;
		}

		void LoadMeasureLengths()
		{
			int Measure = 0;
			auto& lines = root["lines"];
			auto& Measures = Chart->Data->Measures;

			if (lines.isNull()) return;
			if (!lines.isArray()) return;

			if (lines.size())
			{
				// ommitted 0- first measure is...
				if (lines[0]["y"].asDouble() != 0)
				{
					Measures.resize(1);
					Measures[0].Length = lines[0]["y"].asDouble() / resolution;
					Measure++;
				}

				// now get measure lengths appropietly.
				// the first is not getting accounted for since that was looked at just before.
				for (auto msr = lines.begin(); msr != lines.end(); ++msr)
				{
					auto next = msr; ++next;
					if (next != lines.end())
					{
						double duration = ((*next)["y"].asDouble() - (*msr)["y"].asDouble()) / resolution;

						if (Measure >= Measures.size())
							Measures.resize(Measure + 1);

						Measures[Measure].Length = duration;
					}
					Measure++;
				}
			}

			// make Measure point to the last added measure
			Measure -= 1;
			if (Measure == -1) // none? so it's an array huh
			{
				Measure++;
				Measures.resize(1);
			}

			double prev = 0;
			
			for (int i = 0; i < Measure; i++) // set length of last measure to difference between last note and last measure's beats.
				prev += Chart->Data->Measures[i].Length;

			Chart->Data->Measures[Chart->Data->Measures.size() - 1].Length = FindLastNoteBeat() - prev;
		}

		void LoadTiming()
		{
			Json::Value bpm_notes;
			Json::Value stop_notes;

			if (version == UNSPECIFIED_VERSION)
			{
				bpm_notes = root["bpmNotes"];
				stop_notes = root["stopNotes"];
			}
			else if (version == VERSION_1)
			{
				bpm_notes = root["bpm_events"];
				stop_notes = root["stop_events"];
			}

			for (auto bpm : bpm_notes)
			{
				double y = bpm["y"].asDouble() / resolution;
				double val;
				
				if (version == UNSPECIFIED_VERSION)
					val = bpm["v"].asDouble();
				else
					val = bpm["bpm"].asDouble();

				Chart->Timing.push_back(TimingSegment(y, val));
			}

			for (auto stop : stop_notes)
			{
				double y = stop["y"].asDouble() / resolution;
				double val;

				if (version == UNSPECIFIED_VERSION)
					val = spb(SectionValue(Chart->Timing, y)) * stop["v"].asDouble() / resolution;
				else
					val = spb(SectionValue(Chart->Timing, y)) * stop["duration"].asDouble() / resolution;

				Chart->Data->Stops.push_back(TimingSegment(y, val));
			}
		}

		float TimeForObj(double beat)
		{
			return TimeAtBeat(Chart->Timing, 0, beat) + StopTimeAtBeat(Chart->Data->Stops, beat);
		}

		size_t MeasureForBeat(double beat)
		{
			// stub
			double acom = 0;
			size_t Measure = 0;
			while (acom < beat)
			{
				if (Chart->Data->Measures.size() > Measure)
					acom += Chart->Data->Measures[Measure].Length;
				else
					acom += 4;
				Measure++;
			}

			return Measure;
		}

		void GetSoundChannels(Json::Value& snd_channel)
		{
			if (version == UNSPECIFIED_VERSION)
				snd_channel = root["soundChannel"];
			else
				snd_channel = root["sound_channels"];
		}

		int GetMappedLane(Json::Value& values)
		{
			int lane = values["x"].asInt() - 1;
			if (lane < 0)
				return lane;
			if (lane < mappings.size())
				return mappings[lane];
			throw std::exception(Utility::Format("x = %d out of bounds for mode hint", lane).c_str());
		}

		int Slice(int sound_index, double &last_time, Json::Value& notes, Json::Value::iterator& note)
		{
			double st = 0, et = std::numeric_limits<double>::infinity();
			bool noreset = (*note)["c"].asBool();
			auto next = note;
			++next;
			if (noreset)
			{
				if (note != notes.begin())
				{
					st = last_time;
					// if it's not the first note, use the previous note's time if this doesn't reset.
				}

				// if the next is the last note, use what remains of the audio, else
				// use the time between this and the next note as the end cue point
				if (next != notes.end())
					last_time = et = st + TimeForObj((*next)["y"].asDouble() / resolution) - TimeForObj((*note)["y"].asDouble() / resolution);
			}
			else
			{
				st = 0;
				// if there's a note up ahead, use that as the end time
				// otherwise, use the entire audio
				if (next != notes.end())
				{
					double duration = TimeForObj((*next)["y"].asDouble() / resolution) - TimeForObj((*note)["y"].asDouble() / resolution);
					last_time = et = duration;
				}
			}

			// Add this slice as an object or to an existing object
			double note_time = (*note)["y"].asDouble() / resolution;
			int note_pulse = (*note)["y"].asInt();
			int note_x = (*note)["x"].asInt();
			int lane = GetMappedLane(*note);

			// is there a note already at this line, at this time?
			if (Notes[lane].find(note_time) != Notes[lane].end())
			{
				// get the slices for the wav of this note, add this sound's slice to it
				auto &slice = Slices.Slices[Notes[lane][note_time].Sound];

				// check if the slice duration is similar?
				// w/e lol. keep this slice as the last declared if overlap exists.
				slice[sound_index].Start = st;
				slice[sound_index].End = et;

				return Notes[lane][note_time].Sound;
			} else // there's no note on this lane or at this time
			{
				int wav = current_wav;
				// a: TODO: find good criteria to reuse slice

				// b: the slice is different (wav == current_wav is true)
				if (wav == current_wav)
				{
					auto &slice = Slices.Slices[wav];
					slice[sound_index].Start = st;
					slice[sound_index].End = et;
				}

				// add a new object.
				AddObject(sound_index, note, wav);

				if (wav == current_wav)
					current_wav++;

				return wav;
			}
		}

		void AddObject(int sound_index, Json::Value::iterator& note, int wav_index)
		{
			int lane = GetMappedLane(*note);
			double beat = (*note)["y"].asDouble() / resolution;
			double len = (*note)["l"].asDouble() / resolution;

			if (lane < -1) // only for slice purposes?
				return;

			if (lane >= -1)
			{
				Notes[lane][beat].Sound = wav_index;
				Notes[lane][beat].Length = len;
			}
		}

		GString CleanFilename(GString nam)
		{
			nam = regex_replace(nam, std::regex("\\.\\./?"), "");
			// remove C:/... or / or C:\...

			nam = regex_replace(nam, std::regex("^\\s*(.:)?[/\\\\]"), "");

			return nam;
		}

		void AddGlobalSliceSound(GString nam, int sound_index)
		{
			// remove ".."
			nam = CleanFilename(nam);

			// Add to index
			Slices.AudioFiles[sound_index] = nam;
		}

		void LoadNotes()
		{
			int sound_index = 1;
			Json::Value snd_channel;
			GetSoundChannels(snd_channel);

			for (auto audio = snd_channel.begin(); audio != snd_channel.end(); ++audio)
			{
				double last_time = 0;
				AddGlobalSliceSound((*audio)["name"].asString(), sound_index);

				auto notes = (*audio)["notes"];
				for (auto note = notes.begin(); note != notes.end(); ++note)
					Slice(sound_index, last_time, notes, note);
				sound_index += 1;
			}
		}

		void LoadBGA()
		{
			if (version != VERSION_1) return; // BGA unsupported on version 0.21
			auto out = make_shared<VSRG::BMPEventsDetail>();

			auto& bga = root["bga"];
			if (!bga.isNull())
			{
				for (auto &bgi: bga["bga_header"])
					out->BMPList[bgi["id"].asInt()] = CleanFilename(bgi["name"].asString());

				for (auto &bg0 : bga["bga_notes"])
					out->BMPEventsLayerBase.push_back(AutoplayBMP(TimeForObj(bg0["y"].asDouble() / resolution), bg0["id"].asInt()));
				for (auto &bg0 : bga["layer_notes"])
					out->BMPEventsLayer.push_back(AutoplayBMP(TimeForObj(bg0["y"].asDouble() / resolution), bg0["id"].asInt()));
				for (auto &bg0 : bga["poor_notes"])
					out->BMPEventsLayerMiss.push_back(AutoplayBMP(TimeForObj(bg0["y"].asDouble() / resolution), bg0["id"].asInt()));
			}
		}
	public:

		BMSONLoader(std::ifstream &inp, VSRG::Song* out) : input(inp)
		{
			input >> root;
			song = out;

			resolution = BMSON_DEFAULT_RESOLUTION;
			current_wav = 1;
		}

		void SetFilename(GString fn)
		{
			Chart->Filename = fn;
			if (Chart->Name.length() == 0)
				Chart->Name = Utility::RemoveExtension(Directory(fn).Filename().path());
		}

		void AddNotesToDifficulty()
		{
			for (auto &s: Slices.AudioFiles)
				Log::Logf("Track %d: %s\n", s.first, s.second.c_str());
			for (auto lane: Notes)
			{
				for (auto note: lane.second)
				{
					if (lane.first == -1) // lane # is bgm
					{
						Log::Logf("Add BGM track at %f (%f/%f) wav: %d slices: %d\n", TimeForObj(note.first), note.first, note.first * resolution, note.second.Sound, Slices.Slices[note.second.Sound].size());
						for (auto &s : Slices.Slices[note.second.Sound])
						{
							Log::Logf("\tTrack %d Start/End: %f/%f\n", s.first, s.second.Start, s.second.End);
						}
						Chart->Data->BGMEvents.push_back(AutoplaySound(TimeForObj(note.first), note.second.Sound));
						continue;
					}

					VSRG::NoteData new_note;
					new_note.StartTime = TimeForObj(note.first);
					if (note.second.Length)
					{
						new_note.EndTime = TimeForObj(note.first + note.second.Length);
						Chart->TotalHolds++;
						Chart->TotalScoringObjects++;
					}

					new_note.Sound = note.second.Sound;

					int Measure = MeasureForBeat(note.first);
					if (Measure >= Chart->Data->Measures.size())
						Chart->Data->Measures.resize(Measure + 1);
					Chart->Data->Measures[MeasureForBeat(note.first)].Notes[lane.first].push_back(new_note);

					Chart->TotalObjects++;
					Chart->TotalScoringObjects++;
					Chart->TotalNotes++;

					Chart->Duration = max(max(Chart->Duration, new_note.StartTime), new_note.EndTime);
				}
			}
			
		}

		void DoLoad()
		{
			Chart = make_shared<VSRG::Difficulty>();
			Chart->Data = make_shared<VSRG::DifficultyLoadInfo>();
			Chart->Data->TimingInfo = TimingInfo = make_shared<VSRG::BMSTimingInfo>();
			TimingInfo->IsBMSON = true;

			if (!root.isMember("version")) // NSE (check member to be == 1.0.0 for VERSION_1!)
				version = UNSPECIFIED_VERSION;
			else
			{
				if (!root["version"].isNull() && root["version"].asString() == VERSION_1)
					version = VERSION_1;
				else
					throw std::exception(Utility::Format("Unknown BMSON version (%s)", root["version"].asString()).c_str());
			}

			LoadMeta();
			LoadMeasureLengths();
			LoadTiming();
			LoadNotes();
			AddNotesToDifficulty();

			Chart->Data->SliceData = Slices;

			LoadBGA();
			song->Difficulties.push_back(Chart);
		}
	};

	void LoadObjectsFromFile(GString filename, GString prefix, VSRG::Song* Out)
	{
#if (!defined _WIN32)
		std::ifstream filein(filename.c_str());
#else
		std::ifstream filein(Utility::Widen(filename).c_str());
#endif

		BMSONLoader bmson(filein, Out);
		bmson.DoLoad();
		bmson.SetFilename(filename);
	}

}