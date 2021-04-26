#include <filesystem>
#include <rmath.h>

#include <unordered_set>
#include <regex> 
#include <fstream>
#include <utility>
#include <game/Song.h>

#include <TextAndFileUtil.h>
#include <json.hpp>


using Json = nlohmann::json;
using namespace rd;

// All non-standard exceptions are marked with NSE.

namespace NoteLoaderBMS
{
    /* from NoteLoaderBMS */
    std::string GetSubtitles(const std::string& SLine, std::unordered_set<std::string> &Out);
}

namespace NoteLoaderBMSON
{
    const char* UNSPECIFIED_VERSION = "0.21";
    const char* VERSION_1 = "1.0.0";

    // Beat = locate / resolution.
    const double BMSON_DEFAULT_RESOLUTION = 240.0;

    struct BmsonNote
    {
        int Sound; // Slice mapped to.
        double Length; // in beats
    };

    struct
    {
        const char* hint;
        int keys;
        std::vector<int> mappings;
        bool turntable;
    } BmsonLayouts[] = {
        { "beat-7k", 8, { 1, 2, 3, 4, 5, 6, 7, 0 }, true },
        { "beat-5k", 6, { 1, 2, 3, 4, 5, -1, -1, 0 }, true },
        { "beat-10k", 12, { 1, 2, 3, 4, 5, -1, -1, 0, 7, 8, 9, 10, 11, -1, -1, 6 }, true },
        { "beat-14k", 16, { 1, 2, 3, 4, 5, 6, 7, 0, 9, 10, 11, 12, 13, 14, 15, 8 }, true },
        { "popn-5k", 5, { 0, 1, 2, 3, 4 }, false },
        { "popn-9k", 9, { 0, 1, 2, 3, 4, 5, 6, 7, 8 }, false }
    };

    class BMSONException : public std::exception
    {
    private:
	std::string msg; 
    public:
        explicit BMSONException(const char * what) : exception(), msg(what) {}
	[[nodiscard]] const char* what() const noexcept override { return msg.c_str(); }
    };

    struct BmsonObject
    {
        int x; // lane
        double y; // pulse
        double l; // length
        bool c; // continuation flag
    };

    class BMSONLoader
    {
        Json root;
        std::ifstream &input;
        rd::Song* song;
        std::unique_ptr<rd::Difficulty> Chart;
        rd::BMSChartInfo* TimingInfo;
        std::unordered_set<std::string> subtitles;
        std::string version;
        double resolution;

        int current_wav;
        std::vector<int> mappings;

        std::map<int, std::map<double, BmsonNote> > Notes; // int := mapped lane; double := time in beats of obj (for :mix-note)

        SliceContainer Slices;

        std::string GetSubartist(const char *string)
        {
            std::regex sreg(Utility::Format(R"(\s*%s\s*:\s*(.*?)\s*$)", string),
				std::regex_constants::icase | std::regex_constants::ECMAScript);

            for (const auto& s : root["info"]["subartists"])
            {
                std::smatch sm;
                std::string str = s;
                if (regex_search(str, sm, sreg))
                {
                    return sm[1];
                }
            }

            return "";
        }

        void SetChannelsFromModeHint(Json values)
        {
            std::regex generic_keys("generic\\-(\\d+)keys");
            std::regex special_keys("special\\-(\\d+)keys");
            std::smatch sm;

            mappings.clear();

            if (values.is_null())
            {
                // default_layout:
                Chart->Data->Turntable = true;
                Chart->Channels = 8;
                mappings = BmsonLayouts[0].mappings;
                return;
            }

            std::string s = values;
            if (regex_search(s, sm, generic_keys))
            {
                int chans = atoi(sm[1].str().c_str());
                if (chans <= MAX_CHANNELS)
                {
                    Chart->Channels = chans;
                    for (int i = 0; i < chans; i++)
                        mappings.push_back(i);
                    return;
                }
            }

            if (regex_search(s, sm, special_keys))
            {
                int chans = atoi(sm[1].str().c_str());
                if (chans <= rd::MAX_CHANNELS)
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
                if (values == layout.hint)
                {
                    Chart->Channels = layout.keys;
                    mappings = layout.mappings;
                    Chart->Data->Turntable = layout.turntable;
                    return;
                }
            }

            // Okay then, didn't match anything...
            throw BMSONException(Utility::Format("Unknown mode hint: \"%s\"", values.get<std::string>().c_str()).c_str());
        }

        void LoadMeta()
        {
            auto meta = root["info"];
            song->Title = NoteLoaderBMS::GetSubtitles(meta["title"], subtitles);
            song->Artist = meta["artist"];
            song->Subtitle = meta["subtitle"].get<std::string>() + Utility::Join(subtitles, " ");

            song->BackgroundFilename = meta["back_image"].get<std::string>();

            for (auto &s : meta["subtitles"])
                subtitles.insert(s.get<std::string>());

            if (version == UNSPECIFIED_VERSION)
                Chart->Timing.push_back(TimingSegment(0, meta["initBPM"]));
            else if (version == VERSION_1)
            {
                if (meta["init_bpm"].is_null())
                    throw BMSONException("Unspecified init_bpm!");
                Chart->Timing.push_back(TimingSegment(0, meta["init_bpm"]));
            }

            Chart->Level = meta["level"];

            Chart->Author = GetSubartist("chart");

            if (!meta["chart_name"].is_null())
                Chart->Name = meta["chart_name"];

            if (!meta["eyecatch_image"].is_null())
                Chart->Data->StageFile = meta["eyecatch_image"];

            Chart->BPMType = rd::Difficulty::BT_BEAT;

            SetChannelsFromModeHint(meta["mode_hint"]);

            Chart->IsVirtual = true;

            if (!meta["resolution"].is_null())
                resolution = abs(meta["resolution"].get<double>());

            if (resolution == 0) resolution = BMSON_DEFAULT_RESOLUTION;

            song->SongPreviewSource = meta["preview_music"].get<std::string>();

            // DEFEXRANK!
            double jRank;

            Json jr;
            if (version == UNSPECIFIED_VERSION)
                jr = meta["judgeRank"];
            else
                jr = meta["judge_rank"];

            if (!jr.is_null())
                jRank = jr;
            else
                jRank = 100;

			TimingInfo->JudgeRank = jRank;
			TimingInfo->PercentualJudgerank = true;

            TimingInfo->GaugeTotal = meta["total"];
        }

        double FindLastNoteBeat()
        {
            double last_y = -std::numeric_limits<double>::infinity();
            Json *sc;
            GetSoundChannels(sc);
            for (auto &&s : *sc)
            {
                auto notes = s["notes"];
                for (auto &&note : notes)
                {
                    last_y = std::max(note["y"].get<double>(), last_y);
                }
            }

            if (std::isinf(last_y)) return 0;

            return last_y / resolution;
        }

        void LoadMeasureLengths()
        {
            size_t Measure = 0;
            auto& lines = root["lines"];
            auto& Measures = Chart->Data->Measures;

            if (!lines.is_array()) return;

            if (!lines.empty())
            {
                // ommitted 0- first measure is...
                if (lines[0]["y"].get<double>() != 0)
                {
                    Measures.resize(1);
                    Measures[0].Length = lines[0]["y"].get<double>() / resolution;
                    Measure++;
                }

                // now get measure lengths appropietly.
                // the first is not getting accounted for since that was looked at just before.
                for (auto msr = lines.begin(); msr != lines.end(); ++msr)
                {
                    auto next = msr; ++next;
                    if (next != lines.end())
                    {
                        double duration = ((*next)["y"].get<double>() - (*msr)["y"].get<double>()) / resolution;

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

            for (size_t i = 0; i < Measure; i++) // set length of last measure to difference between last note and last measure's beats.
                prev += Chart->Data->Measures[i].Length;

            Chart->Data->Measures[Chart->Data->Measures.size() - 1].Length = FindLastNoteBeat() - prev;
        }

        void LoadTiming()
        {
            Json bpm_notes;
            Json stop_notes;

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
                double y = bpm["y"].get<double>() / resolution;
                double val;

                if (version == UNSPECIFIED_VERSION)
                    val = bpm["v"];
                else
                    val = bpm["bpm"];

                Chart->Timing.push_back(TimingSegment(y, val));
            }

            for (auto stop : stop_notes)
            {
                double y = stop["y"].get<double>() / resolution;
                double val;

                if (version == UNSPECIFIED_VERSION)
                    val = spb(SectionValue(Chart->Timing, y)) * stop["v"].get<double>() / resolution;
                else
                    val = spb(SectionValue(Chart->Timing, y)) * stop["duration"].get<double>() / resolution;

                Chart->Data->Stops.push_back(TimingSegment(y, val));
            }
        }

        double TimeForObj(double beat)
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

        void GetSoundChannels(Json* &snd_channel)
        {
            if (version == UNSPECIFIED_VERSION)
                snd_channel = &root["soundChannel"];
            else
                snd_channel = &root["sound_channels"];
        }

        int GetMappedLane(BmsonObject& note)
        {
            int lane = note.x - 1;
            if (lane < 0)
                return lane;
            if (lane < mappings.size())
                return mappings[lane];
            throw BMSONException(Utility::Format("x = %d out of bounds for mode hint", lane).c_str());
        }

        int Slice(int sound_index, double &last_time, std::vector<BmsonObject>& notes, std::vector<BmsonObject>::iterator& note)
        {
            double st = 0, et = std::numeric_limits<double>::infinity();
            auto next = note + 1;
            if (note->c)
            {
                if (note != notes.begin())
                {
                    st = last_time;
                    // if it's not the first note, use the previous note's time if this doesn't reset.
                }

                // if the next is the last note, use what remains of the audio, else
                // use the time between this and the next note as the end cue point
                if (next != notes.end())
                    last_time = et = st + TimeForObj(next->y / resolution) - TimeForObj(note->y / resolution);
            }
            else
            {
                st = 0;
                // if there's a note up ahead, use that as the end time
                // otherwise, use the entire audio
                if (next != notes.end())
                {
                    double duration = TimeForObj(next->y / resolution) - TimeForObj(note->y / resolution);
                    last_time = et = duration;
                }
            }

            // Add this slice as an object or to an existing object
            double note_time = note->y / resolution;
            int lane = GetMappedLane(*note);

            // is there a note already at this line, at this time?
            // (:mix-note)
            if (Notes[lane].find(note_time) != Notes[lane].end())
            {
                // get the slices for the wav of this note, add this sound's slice to it
                auto &slice = Slices.Slices[Notes[lane][note_time].Sound];

                // check if the slice duration is similar?
                // w/e lol. keep this slice as the last declared if overlap exists.
                slice[sound_index].Start = st;
                slice[sound_index].End = et;

                return Notes[lane][note_time].Sound;
            }
            else // there's no note on this lane or at this time
            {
                int wav = current_wav;
                // a: TODO: find good criteria to reuse slice

                // b: the slice is different (wav == current_wav is true)
                auto &slice = Slices.Slices[wav];
                slice[sound_index].Start = st;
                slice[sound_index].End = et;

                // add a new object.
                AddObject(sound_index, note, wav);

                if (wav == current_wav)
                    current_wav++;

                return wav;
            }
        }

        void AddObject(int sound_index, std::vector<BmsonObject>::iterator& note, int wav_index)
        {
            int lane = GetMappedLane(*note);
            double beat = note->y / resolution;
            double len = note->l / resolution;

            if (lane < -1) // only for slice purposes?
                return;

            Notes[lane][beat].Sound = wav_index;
            Notes[lane][beat].Length = len;            
        }

        std::string CleanFilename(std::string nam)
        {
            nam = regex_replace(nam, std::regex("\\.\\./?"), "");
            // remove C:/... or / or C:\...

            nam = regex_replace(nam, std::regex(R"(^\s*(.:)?[/\\])"), "");

            return nam;
        }

        void AddGlobalSliceSound(std::string nam, int sound_index)
        {
            // remove ".."
            nam = CleanFilename(nam);

            // Add to index
            Slices.AudioFiles[sound_index] = nam;
        }

        void JoinBGMSlices(std::vector<BmsonObject> &objs)
        {
            for (auto obj = objs.begin(); obj != objs.end(); )
            {
                if (obj->x == 0)
                {
                    // BGM track? No need to have them separated, join them.
                    auto ni = obj + 1;

                    // It's a BGM track and it doesn't restart audio?
                    while (ni != objs.end() && ni->x == 0 && ni->c)
                    {
                        ni = objs.erase(ni);
                    }

                    obj = ni;
                    continue;
                }

                ++obj;
            }
        }

        void LoadNotes()
        {
            int sound_index = 1;
            Json *snd_channel;
            GetSoundChannels(snd_channel);

            for (auto & audio : *snd_channel)
            {
                double last_time = 0;
                AddGlobalSliceSound(audio["name"], sound_index);

                std::vector<BmsonObject> objs;

                auto notes = audio["notes"];
                for (auto &note : notes)
                    objs.push_back(BmsonObject
				{ 
					note["x"], 
					note["y"], 
					note["l"], 
					note["c"]
				});

                std::stable_sort(objs.begin(), objs.end(), 
					[](const BmsonObject& l, const BmsonObject& r) -> bool { 
					return l.y < r.y; 
				});
                JoinBGMSlices(objs);

                for (auto note = objs.begin(); note != objs.end(); ++note)
                    Slice(sound_index, last_time, objs, note);
                sound_index += 1;
            }
        }

        void LoadBGA()
        {
            if (version != VERSION_1) return; // BGA unsupported on version 0.21
            auto out = std::make_shared<rd::BMPEventsDetail>();

            auto& bga = root["bga"];
            if (!bga.is_null())
            {
				bool hasData = false;
                for (auto &bgi : bga["bga_header"])
                    out->BMPList[bgi["id"]] = CleanFilename(bgi["name"]);

				if (version != VERSION_1) {
					for (auto &bg0 : bga["bga_notes"]) {
						out->BMPEventsLayerBase.emplace_back(
                            AutoplayBMP(
                                TimeForObj(bg0["y"].get<double>() / resolution), 
                                bg0["id"]
                            )
                        );
						hasData = true;
					}
					for (auto &bg0 : bga["layer_notes"]) {
						out->BMPEventsLayer.emplace_back(
                            AutoplayBMP(
                                TimeForObj(bg0["y"].get<double>() / resolution), 
                                bg0["id"]
                            )
                        );
						hasData = true;
					}
					for (auto &bg0 : bga["poor_notes"]) {
						out->BMPEventsLayerMiss.emplace_back(
                            AutoplayBMP(
                                TimeForObj(bg0["y"].get<double>() / resolution), 
                                bg0["id"]
                            )
                        );

						hasData = true;
					}
				}
				else {
					for (auto &bg0 : bga["bga_events"]) {
						out->BMPEventsLayerBase.emplace_back(
                            AutoplayBMP(
                                TimeForObj(bg0["y"].get<double>() / resolution), 
                                bg0["id"]
                            )
                        )
                        ;
						hasData = true;
					}
					for (auto &bg0 : bga["layer_events"]) {
						out->BMPEventsLayer.emplace_back(
                            AutoplayBMP(
                                TimeForObj(bg0["y"].get<double>() / resolution), 
                                bg0["id"]
                            )
                        );
						hasData = true;
					}
					for (auto &bg0 : bga["poor_events"]) {
						out->BMPEventsLayerMiss.emplace_back(
                            AutoplayBMP(
                                TimeForObj(bg0["y"].get<double>() / resolution), 
                                bg0["id"]
                            )
                        );
						hasData = true;
					}
				}

				if (hasData)
					Chart->Data->BMPEvents = out;
            }
        }
    public:

        BMSONLoader(std::ifstream &inp, rd::Song* out) : input(inp)
        {
            input >> root;
            song = out;

            resolution = BMSON_DEFAULT_RESOLUTION;
            current_wav = 1;
        }

        void SetFilename(std::filesystem::path fn)
        {
            Chart->Filename = std::move(fn);
			auto fx = Chart->Filename;
            if (Chart->Name.length() == 0)
                Chart->Name = Conversion::ToU8(fx.filename().replace_extension("").wstring());
        }

        void AddNotesToDifficulty()
        {
            // for (auto &s : Slices.AudioFiles)
            //    Log::Logf("Track %d: %s\n", s.first, s.second.c_str());
            for (const auto& lane : Notes)
            {
                for (auto note : lane.second)
                {
                    if (lane.first == -1) // lane # is bgm
                    {
                        // Log::Logf("Add BGM track at %f (%f/%f) wav: %d slices: %d\n", TimeForObj(note.first), note.first, note.first * resolution, note.second.Sound, Slices.Slices[note.second.Sound].size());
                        for (auto &s : Slices.Slices[note.second.Sound])
                        {
                            // Log::Logf("\tTrack %d Start/End: %f/%f\n", s.first, s.second.Start, s.second.End);
                        }
                        Chart->Data->BGMEvents.emplace_back(AutoplaySound(TimeForObj(note.first), note.second.Sound));
                        continue;
                    }

                    NoteData new_note;
                    new_note.StartTime = TimeForObj(note.first);
                    if (note.second.Length)
                    {
                        new_note.EndTime = TimeForObj(note.first + note.second.Length);
                    }

                    new_note.Sound = note.second.Sound;

                    auto Measure = MeasureForBeat(note.first);
                    if (Measure >= Chart->Data->Measures.size())
                        Chart->Data->Measures.resize(Measure + 1);
                    Chart->Data->Measures[MeasureForBeat(note.first)].Notes[lane.first].push_back(new_note);

                    Chart->Duration = std::max(std::max(Chart->Duration, new_note.StartTime), new_note.EndTime);
                }
            }
        }

        void DoLoad()
        {
            Chart = std::make_unique<rd::Difficulty>();
            Chart->Data = std::make_unique<rd::DifficultyLoadInfo>();
            Chart->Data->TimingInfo = std::make_unique<rd::BMSChartInfo>();
            TimingInfo = static_cast<rd::BMSChartInfo*>(Chart->Data->TimingInfo.get());
            TimingInfo->IsBMSON = true;

            if (root["version"].is_null()) // NSE (check member to be == 1.0.0 for VERSION_1!)
                version = UNSPECIFIED_VERSION;
            else
            {
                if (root["version"].get<std::string>() == VERSION_1)
                    version = VERSION_1;
                else
                    throw BMSONException(
                        Utility::Format(
                            "Unknown BMSON version (%s)", 
                        root["version"].get<std::string>().c_str()
                        ).c_str()
                        );
            }

            LoadMeta();
            LoadMeasureLengths();
            LoadTiming();
            LoadNotes();
            AddNotesToDifficulty();

            Chart->Data->SliceData = Slices;

            LoadBGA();
            song->Difficulties.push_back(std::move(Chart));
        }
    };

    void LoadObjectsFromFile(const std::filesystem::path& filename, rd::Song* Out)
    {
		std::ifstream filein(filename);

        BMSONLoader bmson(filein, Out);
        bmson.DoLoad();
        bmson.SetFilename(filename);
    }
}
