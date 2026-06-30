#include <game/OtowormLoaderBridge.h>

#include <note_loader_7k.h>

namespace
{
    TimingSegment convert_segment(const otoworm::TimingSegment& segment)
    {
        return {segment.time, segment.value};
    }

    TimingData convert_timing(const otoworm::TimingData& timing)
    {
        TimingData out;
        out.reserve(timing.size());
        for (const auto& segment : timing)
            out.push_back(convert_segment(segment));
        return out;
    }

    TimingData convert_bps_to_time_timing(const otoworm::TimingData& bps)
    {
        TimingData out;
        out.reserve(bps.size());
        for (const auto& segment : bps)
            out.emplace_back(segment.time, segment.value * 60.0);
        return out;
    }

    AutoplaySound convert_autoplay_sound(const otoworm::AutoplaySound& sound)
    {
        return {sound.time, sound.sound};
    }

    AutoplayBMP convert_autoplay_bmp(const otoworm::AutoplayBMP& bmp)
    {
        return {bmp.time, bmp.bmp};
    }

    SliceInfo convert_slice_info(const otoworm::SliceInfo& slice)
    {
        return {slice.start, slice.end};
    }

    SliceContainer convert_slice_container(const otoworm::SliceContainer& slices)
    {
        SliceContainer out;
        out.AudioFiles = slices.audio_files;
        for (const auto& [audio_index, audio_slices] : slices.slices)
        {
            for (const auto& [slice_index, slice] : audio_slices)
                out.Slices[audio_index][slice_index] = convert_slice_info(slice);
        }
        return out;
    }

    rd::NoteData convert_note(const otoworm::NoteData& note)
    {
        rd::NoteData out;
        out.StartTime = note.start;
        out.EndTime = note.end_time;
        out.Sound = note.sound;
        out.TailSound = note.tail_sound;
        out.NoteKind = note.type;
        return out;
    }

    rd::Measure convert_measure(const otoworm::Measure& measure)
    {
        rd::Measure out;
        out.Length = measure.length;
        for (size_t channel = 0; channel < otoworm::MAX_CHANNELS; ++channel)
        {
            out.Notes[channel].reserve(measure.notes[channel].size());
            for (const auto& note : measure.notes[channel])
                out.Notes[channel].push_back(convert_note(note));
        }
        return out;
    }

    rd::SpeedSection convert_speed_section(const otoworm::SpeedSection& section)
    {
        rd::SpeedSection out;
        out.Time = section.time;
        out.Duration = section.duration;
        out.Value = section.value;
        out.IntegrateByBeats = section.use_beat_integration;
        return out;
    }

    std::shared_ptr<rd::ChartInfo> convert_chart_info(const std::shared_ptr<otoworm::ChartInfo>& info)
    {
        if (!info)
            return nullptr;

        switch (info->get_class())
        {
        case otoworm::CC_BMS:
        {
            const auto bms = std::static_pointer_cast<otoworm::BMSChartInfo>(info);
            auto out = std::make_shared<rd::BMSChartInfo>();
            out->JudgeRank = bms->judge_rank;
            out->GaugeTotal = bms->gauge_total;
            out->PercentualJudgerank = bms->percentual_judgerank;
            out->IsBMSON = bms->is_bmson;
            return out;
        }
        case otoworm::CC_OSUMANIA:
        {
            const auto mania = std::static_pointer_cast<otoworm::OsumaniaChartInfo>(info);
            auto out = std::make_shared<rd::OsumaniaChartInfo>();
            out->HP = mania->hp;
            out->OD = mania->od;
            return out;
        }
        case otoworm::CC_O2JAM:
        {
            const auto o2 = std::static_pointer_cast<otoworm::O2JamChartInfo>(info);
            auto out = std::make_shared<rd::O2JamChartInfo>();
            out->Difficulty = static_cast<decltype(out->Difficulty)>(o2->difficulty);
            return out;
        }
        case otoworm::CC_STEPMANIA:
            return std::make_shared<rd::StepmaniaChartInfo>();
        case otoworm::CC_NULL:
            return nullptr;
        }

        return nullptr;
    }

    std::shared_ptr<rd::BMPEventsDetail> convert_bmp_events(const std::optional<otoworm::BMPEventsDetail>& bmp_events)
    {
        if (!bmp_events)
            return nullptr;

        auto out = std::make_shared<rd::BMPEventsDetail>();
        out->BMPList = bmp_events->bmp_list;

        for (const auto& event : bmp_events->layer_base)
            out->BMPEventsLayerBase.push_back(convert_autoplay_bmp(event));
        for (const auto& event : bmp_events->layer_upper)
            out->BMPEventsLayer.push_back(convert_autoplay_bmp(event));
        for (const auto& event : bmp_events->layer_upper2)
            out->BMPEventsLayer2.push_back(convert_autoplay_bmp(event));
        for (const auto& event : bmp_events->layer_miss)
            out->BMPEventsLayerMiss.push_back(convert_autoplay_bmp(event));

        return out;
    }

    std::unique_ptr<rd::DifficultyLoadInfo> convert_transient(const otoworm::ChartTransient& transient)
    {
        auto out = std::make_unique<rd::DifficultyLoadInfo>();
        out->Scrolls = convert_timing(transient.scrolls);
        out->Warps = convert_timing(transient.warps);

        out->Measures.reserve(transient.measures.size());
        for (const auto& measure : transient.measures)
            out->Measures.push_back(convert_measure(measure));

        out->InterpoloatedSpeedMultipliers.reserve(transient.interpolated_speed_multipliers.size());
        for (const auto& section : transient.interpolated_speed_multipliers)
            out->InterpoloatedSpeedMultipliers.push_back(convert_speed_section(section));

        out->BGMEvents.reserve(transient.bgm_events.size());
        for (const auto& sound : transient.bgm_events)
            out->BGMEvents.push_back(convert_autoplay_sound(sound));

        out->BMPEvents = convert_bmp_events(transient.bmp_events);
        out->TimingInfo = convert_chart_info(transient.specialized_info);
        out->SoundList.insert(transient.sound_list.begin(), transient.sound_list.end());
        out->StageFile = transient.stage_file;
        out->Genre = transient.genre;
        out->Turntable = transient.has_turntable;
        out->FileHash = transient.file_hash;
        out->IndexInFile = transient.index_in_file;
        out->SliceData = convert_slice_container(transient.slice_data);

        if (const auto mania = dynamic_cast<const otoworm::OsumaniaChartTransient*>(&transient))
            out->osbSprites = mania->osb_sprites;

        return out;
    }

    std::shared_ptr<rd::Difficulty> convert_chart(const std::shared_ptr<otoworm::Chart>& chart_ptr)
    {
        const auto& chart = *chart_ptr;
        auto out = std::make_shared<rd::Difficulty>();
        out->OtoChart = chart_ptr;
        out->Duration = chart.duration;
        out->ID = chart.id == static_cast<size_t>(-1) ? -1 : static_cast<int>(chart.id);

        if (chart.meta)
        {
            out->Name = chart.meta->name;
            out->Filename = chart.meta->path;
            out->Author = chart.meta->author;
        }

        out->Level = chart.level;
        out->Channels = chart.channels;
        out->IsVirtual = chart.has_no_audio_stream;
        out->BPMType = rd::Difficulty::BT_MS;

        if (chart.transient)
        {
            out->Offset = 0;
            out->Timing = convert_bps_to_time_timing(chart.transient->bps);
            out->Data = convert_transient(*chart.transient);
        }
        else
        {
            out->Offset = chart.offset;
            out->Data = std::make_unique<rd::DifficultyLoadInfo>();
        }

        return out;
    }

}

namespace rd
{
    void ConvertFromOtoworm(std::shared_ptr<otoworm::ChartGroup> source_ptr, Song* out)
    {
        const auto& source = *source_ptr;
        out->OtoChartGroup = std::move(source_ptr);
        out->ID = source.id;
        out->Title = source.title;
        out->Artist = source.artist;
        out->SongDirectory = source.path;
        out->SongFilename = source.song_filename;
        out->BackgroundFilename = source.background_filename;
        out->SongPreviewSource = source.song_preview_source;
        out->PreviewTime = source.preview_time;
        out->Subtitle = source.subtitle;
        out->Genre = source.genre;

        out->Difficulties.clear();
        out->Difficulties.reserve(source.charts.size());
        for (const auto& chart : source.charts)
        {
            if (chart)
                out->Difficulties.push_back(convert_chart(chart));
        }
    }

    void ConvertFromOtoworm(const otoworm::ChartGroup& source, Song* out)
    {
        auto source_copy = std::make_shared<otoworm::ChartGroup>(source);
        ConvertFromOtoworm(std::move(source_copy), out);
    }
}
