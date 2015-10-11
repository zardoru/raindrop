#include <json/json.h>
#include <fstream>
#include <unordered_set>

#include "GameGlobal.h"
#include "Song7K.h"

namespace NoteLoaderBMS
{
	GString GetSubtitles(GString SLine, std::unordered_set<GString> &Out);
}

namespace NoteLoaderBMSON{

	// Beat = locate / resolution. 
	const double BMSON_DEFAULT_RESOLUTION = 240.0;

	const struct
	{
		int bmson_level;
		int rank_level;
	} level_bindings [] = {
		{52, 1},
		{60, 2},
		{70, 3},
		{100, 4},
		{120, 5}
	};

	class BMSONLoader {
		Json::Value root;
		std::ifstream &input;
		VSRG::Song* song;
		shared_ptr<VSRG::Difficulty> Chart;
		shared_ptr<VSRG::BMSTimingInfo> TimingInfo;
		std::unordered_set<GString> subtitles;

		double resolution;

		void LoadMeta()
		{
			auto meta = root["info"];
			song->SongName = NoteLoaderBMS::GetSubtitles(meta["title"].asString(), subtitles);
			song->SongAuthor = meta["artist"].asString();
			song->Subtitle = boost::join(subtitles, " ");

			Chart->Timing.push_back(TimingSegment(0, meta["initBPM"].asDouble()));
			Chart->Level = meta["level"].asInt();
			Chart->Author = meta["maker"].asString();

			if (!meta["chartname"].isNull())
				Chart->Name = meta["chartname"].asString();

			Chart->BPMType = VSRG::Difficulty::BT_BEAT;

			if (meta["channels"].isNull())
				Chart->Channels = 8;
			else
				Chart->Channels = Clamp(meta["channels"].asInt(), 0, VSRG::MAX_CHANNELS - 1);

			Chart->IsVirtual = true;

			if (!meta["resolution"].isNull())
				resolution = meta["resolution"].asDouble() / 4;

			// DEFEXRANK?
			int jRank = meta["judgeRank"].asInt();
			for (auto v : level_bindings)
				if (v.bmson_level == jRank)
					TimingInfo->JudgeRank = v.rank_level;
			TimingInfo->GaugeTotal = meta["total"].asInt();
		}

		void LoadMeasureLengths()
		{
			int Measure = 0;
			for (auto msr = root["lines"].begin(); msr != root["lines"].end(); ++msr)
			{
				auto next = msr; ++next;
				if (next != root["lines"].end())
				{
					double duration = ((*next)["y"].asDouble() - (*msr)["y"].asDouble()) / resolution;

					if (Measure >= Chart->Data->Measures.size())
						Chart->Data->Measures.resize(Measure + 1);

					Chart->Data->Measures[Measure].Length = duration;
				}
				Measure++;
			}
		}

		void LoadTiming()
		{
			for (auto bpm : root["bpmNotes"])
			{
				double y = bpm["y"].asDouble() / resolution;
				double val = bpm["v"].asDouble();
				Chart->Timing.push_back(TimingSegment(y, val));
			}

			for (auto stop : root["stopNotes"])
			{
				double y = stop["y"].asDouble() / resolution;
				double val = stop["v"].asDouble();
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

		void LoadNotes()
		{
			int sound_index = 1;

			for (auto audio = root["soundChannel"].begin(); audio != root["soundChannel"].end(); ++audio)
			{
				double last_time = 0;
				Chart->SoundList[sound_index] = (*audio)["name"].asString();

				auto notes = (*audio)["notes"];
				for (auto note = notes.begin(); note != notes.end(); ++note)
				{
					int lane = (*note)["x"].asInt();
					double beat = (*note)["y"].asDouble() / resolution;
					double endbeat = ((*note)["y"].asDouble() + (*note)["l"].asDouble()) / resolution;
					bool noreset = (*note)["c"].asBool();

					double st = 0, et = std::numeric_limits<double>::infinity();
					auto prev = note;
					auto next = note;
					if (noreset)
					{
						if (note != notes.begin())
						{
							--prev; // Use previous sound (note - 1 notation doesn't work?)
							st = last_time;
							// if it's not the first note, use the previous note's time if this doesn't reset.
						}

						// if the next is the last note, use what remains of the audio, else
						// use the time between this and the next note as the end cue point
						++next;
						if (next != notes.end())
							last_time = et = st + TimeForObj((*next)["y"].asDouble() / resolution) - TimeForObj((*note)["y"].asDouble() / resolution);
					}
					else
					{
						++next;

						st = 0;
						// if there's a note up ahead, use that as the end time
						// otherwise, use the entire audio
						if (next != notes.end())
						{
							double duration = TimeForObj((*next)["y"].asDouble() / resolution) - TimeForObj((*note)["y"].asDouble() / resolution);
							last_time = et = duration;
						}
					}

					if (lane == 0)
					{
						Chart->Data->BGMEvents.push_back(AutoplaySound(TimeForObj(beat), sound_index, st, et));
						continue;
					}

					if (lane < 0) continue; // for slice purposes?

					const int l2rc[] = { 0, 1, 2, 3, 4, 5, 6, 7, 0, 8, 9, 10, 11, 12, 13, 14, 15 };
					if (lane > VSRG::MAX_CHANNELS && l2rc[lane] >= Chart->Channels)
						throw std::exception("lane out of bounds");

					VSRG::NoteData Note;
					Note.Sound = sound_index;
					Note.AudioStart = st;
					Note.AudioEnd = et;
					Note.StartTime = TimeForObj(beat);

					if (endbeat != beat)
						Note.EndTime = TimeForObj(endbeat);

					int Measure = MeasureForBeat(beat);
					if (Measure >= Chart->Data->Measures.size())
						Chart->Data->Measures.resize(Measure + 1);

					Chart->Data->Measures[Measure].Notes[l2rc[lane]].push_back(Note);

					Chart->Duration = max(max(Chart->Duration, Note.StartTime), Note.EndTime);
					Chart->TotalNotes++;
					Chart->TotalObjects++;
					Chart->TotalScoringObjects++;;
				}
				sound_index += 1;
			}
		}

		void LoadBGA()
		{
			// stub until examples come from wosderge
			if (!root["bga"].isNull())
			{
				if (!root["bga"]["bgaHeader"][0].isNull())
					song->BackgroundFilename = root["bga"]["bgaHeader"][0]["name"].asString();
			}
		}
	public:

		BMSONLoader(std::ifstream &inp, VSRG::Song* out) : input(inp)
		{
			input >> root;
			song = out;

			resolution = BMSON_DEFAULT_RESOLUTION;
		}

		void SetFilename(GString fn)
		{
			Chart->Filename = fn;
			if (Chart->Name.length() == 0)
				Chart->Name = Utility::RemoveExtension(Directory(fn).Filename().path());
		}

		void DoLoad()
		{
			Chart = make_shared<VSRG::Difficulty>();
			Chart->Data = make_shared<VSRG::DifficultyLoadInfo>();
			Chart->Data->TimingInfo = TimingInfo = make_shared<VSRG::BMSTimingInfo>();
			TimingInfo->IsBMSON = true;
			Chart->Data->Turntable = true;

			LoadMeta();
			LoadMeasureLengths();
			LoadTiming();
			LoadNotes();
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