#include <game/RaindropProcessedChart.h>

#include <algorithm>
#include <cmath>

namespace rd {
    namespace {
        constexpr uint8_t EnabledFlag = 1 << 0;
        constexpr uint8_t WasHitFlag = 1 << 1;
        constexpr uint8_t HeadEnabledFlag = 1 << 2;
        constexpr uint8_t FailedHitFlag = 1 << 3;
        constexpr uint8_t InvisibleFlag = 1 << 4;

        int get_fraction_kind_beat(double frac) {
            frac = std::abs(frac);
            if (std::abs(frac - 0.0) < 0.001) return 0;
            if (std::abs(frac - 0.5) < 0.001) return 1;
            if (std::abs(frac - 0.25) < 0.001 || std::abs(frac - 0.75) < 0.001) return 2;
            if (std::abs(frac - 1.0 / 3.0) < 0.001 || std::abs(frac - 2.0 / 3.0) < 0.001) return 3;
            if (std::abs(frac - 1.0 / 6.0) < 0.001 || std::abs(frac - 5.0 / 6.0) < 0.001) return 4;
            if (std::abs(frac - 0.125) < 0.001 || std::abs(frac - 0.375) < 0.001 ||
                std::abs(frac - 0.625) < 0.001 || std::abs(frac - 0.875) < 0.001) return 5;
            return 6;
        }
    }

    RuntimeNote::RuntimeNote() : storage(nullptr), index(0) {}

    RuntimeNote::RuntimeNote(RuntimeNoteStorage* storage, const uint32_t index)
        : storage(storage), index(index) {}

    double& RuntimeNote::get_data_start_time() { return storage->start_time[index]; }
    double& RuntimeNote::get_data_end_time() { return storage->end_time[index]; }
    uint32_t& RuntimeNote::get_data_sound() { return storage->sound[index]; }
    uint8_t RuntimeNote::get_data_note_kind() const { return storage->note_kind[index]; }
    uint8_t RuntimeNote::get_data_fraction_kind() const { return storage->fraction_kind[index]; }

    void RuntimeNote::assign_from(otoworm::TrackNote& note) {
        storage->start_time[index] = note.get_start_time();
        storage->end_time[index] = note.is_hold() ? note.get_end_time() : 0;
        storage->vertical[index] = note.get_vertical();
        storage->hold_end_vertical[index] = note.get_hold_end_vertical();
        storage->sound[index] = note.get_sound();
        storage->tail_sound[index] = note.get_tail_sound();
        storage->note_kind[index] = note.get_data_note_kind();
        storage->fraction_kind[index] = note.get_data_fraction_kind();
        reset();
    }

    void RuntimeNote::assign_position(const double position, const double end_position) {
        storage->vertical[index] = position;
        storage->hold_end_vertical[index] = end_position;
    }

    void RuntimeNote::assign_fraction(const double fraction) {
        storage->fraction_kind[index] = get_fraction_kind_beat(fraction);
    }

    void RuntimeNote::hit() { storage->flags[index] |= WasHitFlag; }

    void RuntimeNote::add_time(const double time) {
        storage->start_time[index] += time;
        if (is_hold())
            storage->end_time[index] += time;
    }

    void RuntimeNote::disable() {
        storage->flags[index] &= ~EnabledFlag;
        disable_head();
    }

    void RuntimeNote::disable_head() {
        storage->flags[index] &= ~HeadEnabledFlag;
    }

    float RuntimeNote::get_vertical() const {
        return static_cast<float>(storage->vertical[index]);
    }

    float RuntimeNote::get_vertical_hold() const {
        return get_vertical() + get_hold_size() / 2;
    }

    bool RuntimeNote::is_hold() const {
        return storage->end_time[index] != 0;
    }

    bool RuntimeNote::is_enabled() const {
        return (storage->flags[index] & EnabledFlag) != 0;
    }

    bool RuntimeNote::is_head_enabled() const {
        return (storage->flags[index] & HeadEnabledFlag) != 0;
    }

    bool RuntimeNote::was_hit() const {
        return (storage->flags[index] & WasHitFlag) != 0;
    }

    bool RuntimeNote::is_judgable() const {
        return storage->note_kind[index] != NK_INVISIBLE && storage->note_kind[index] != NK_FAKE;
    }

    bool RuntimeNote::is_visible() const {
        return storage->note_kind[index] != NK_INVISIBLE && !(storage->flags[index] & InvisibleFlag);
    }

    uint32_t RuntimeNote::get_sound() const { return storage->sound[index]; }
    uint32_t RuntimeNote::get_tail_sound() const { return storage->tail_sound[index]; }

    double RuntimeNote::get_end_time() const {
        return std::max(storage->start_time[index], storage->end_time[index]);
    }

    double RuntimeNote::get_start_time() const {
        return storage->start_time[index];
    }

    int RuntimeNote::get_frac_kind() const {
        return storage->fraction_kind[index];
    }

    float RuntimeNote::get_hold_size() const {
        return std::abs(static_cast<float>(storage->hold_end_vertical[index] - storage->vertical[index]));
    }

    float RuntimeNote::get_hold_end_vertical() const {
        return static_cast<float>(storage->hold_end_vertical[index]);
    }

    void RuntimeNote::fail_hit() {
        storage->flags[index] |= FailedHitFlag;
    }

    bool RuntimeNote::failed_hit() const {
        return (storage->flags[index] & FailedHitFlag) != 0;
    }

    void RuntimeNote::make_invisible() {
        storage->flags[index] |= InvisibleFlag;
    }

    void RuntimeNote::remove_sound() {
        storage->sound[index] = 0;
    }

    void RuntimeNote::reset() {
        storage->flags[index] = EnabledFlag | HeadEnabledFlag;
    }

    void RuntimeNoteStorage::clear() {
        start_time.clear();
        end_time.clear();
        vertical.clear();
        hold_end_vertical.clear();
        sound.clear();
        tail_sound.clear();
        note_kind.clear();
        fraction_kind.clear();
        flags.clear();
        handles.clear();
    }

    uint32_t RuntimeNoteStorage::size() const {
        return static_cast<uint32_t>(start_time.size());
    }

    RuntimeNote& RuntimeNoteStorage::push_note(otoworm::TrackNote& note) {
        const auto index = size();
        start_time.push_back(0);
        end_time.push_back(0);
        vertical.push_back(0);
        hold_end_vertical.push_back(0);
        sound.push_back(0);
        tail_sound.push_back(0);
        note_kind.push_back(NK_NORMAL);
        fraction_kind.push_back(0);
        flags.push_back(EnabledFlag | HeadEnabledFlag);
        handles.emplace_back(this, index);
        handles.back().assign_from(note);
        return handles.back();
    }

    void RuntimeNoteStorage::rebuild_handles() {
        handles.clear();
        handles.reserve(size());
        for (uint32_t i = 0; i < size(); ++i)
            handles.emplace_back(this, i);
    }

    RuntimeNote* RuntimeNoteStorage::note_at(const uint32_t handle) {
        return handle < handles.size() ? &handles[handle] : nullptr;
    }

    const RuntimeNote* RuntimeNoteStorage::note_at(const uint32_t handle) const {
        return handle < handles.size() ? &handles[handle] : nullptr;
    }

    void RuntimeNoteStorage::swap_notes(const uint32_t a, const uint32_t b) {
        using std::swap;
        swap(start_time[a], start_time[b]);
        swap(end_time[a], end_time[b]);
        swap(vertical[a], vertical[b]);
        swap(hold_end_vertical[a], hold_end_vertical[b]);
        swap(sound[a], sound[b]);
        swap(tail_sound[a], tail_sound[b]);
        swap(note_kind[a], note_kind[b]);
        swap(fraction_kind[a], fraction_kind[b]);
        swap(flags[a], flags[b]);
    }

    RaindropProcessedChart::RaindropProcessedChart(const double wait_time)
        : otoworm::ProcessedChart(wait_time) {}

    RaindropProcessedChart RaindropProcessedChart::from(otoworm::Chart* chart, const double speed) {
        RaindropProcessedChart out(DEFAULT_WAIT_TIME);
        auto processed = otoworm::ProcessedChart::from(chart, speed);

        out.speeds = std::move(processed.speeds);
        out.bps = std::move(processed.bps);
        out.warps = std::move(processed.warps);
        out.interpolated_speed_multipliers = std::move(processed.interpolated_speed_multipliers);
        out.has_negative_scroll = processed.has_negative_scroll;
        out.has_turntable = processed.has_turntable;
        out.wait_time = processed.wait_time;
        out.chart = processed.chart;

        for (uint32_t channel = 0; channel < otoworm::MAX_CHANNELS; ++channel) {
            out.notes[channel].clear();
            for (auto& note : processed.notes[channel])
                out.notes[channel].push_note(note);
        }

        out.barlines = out.generate_measure_lines();
        return out;
    }

    std::vector<double> RaindropProcessedChart::generate_measure_lines() const {
        auto& diff = chart;
        const auto& data = diff->transient;
        double last = 0;

        std::vector<double> out;

        if (!data || bps.empty() || data->measures.empty())
            return out;

        const double initial_bps = bps.front().value;
        const double pre_time = wait_time + diff->offset;
        const double pre_time_beats = initial_bps * pre_time;
        const int total_measures = pre_time_beats / data->measures[0].length;
        const double measure_time = 1 / initial_bps * data->measures[0].length;

        for (auto i = 0; i < total_measures; i++) {
            const auto time = diff->offset - measure_time * i;
            out.push_back(speeds.integrate_to_time(time));
        }

        for (const auto& measure : data->measures) {
            out.push_back(speeds.integrate_to_time(get_time_for_beat(last)));
            last += measure.length;
        }

        return out;
    }

    void RaindropProcessedChart::prepare_ordered_notes() {
        for (uint32_t channel = 0; channel < otoworm::MAX_CHANNELS; ++channel) {
            notes[channel].rebuild_handles();
            notes_vertically_ordered[channel].clear();
            notes_time_ordered[channel].clear();

            notes_vertically_ordered[channel].reserve(notes[channel].handles.size());
            notes_time_ordered[channel].reserve(notes[channel].handles.size());
            for (RuntimeNoteHandle handle = 0; handle < notes[channel].size(); ++handle) {
                notes_vertically_ordered[channel].push_back(handle);
                notes_time_ordered[channel].push_back(handle);
            }

            std::ranges::stable_sort(notes_vertically_ordered[channel]
                                     ,
                                     [&](const RuntimeNoteHandle a, const RuntimeNoteHandle b) {
                                         return notes[channel].note_at(a)->get_vertical() < notes[channel].note_at(b)->get_vertical();
                                     });

            std::ranges::stable_sort(notes_time_ordered[channel]
                                     ,
                                     [&](const RuntimeNoteHandle a, const RuntimeNoteHandle b) {
                                         return notes[channel].note_at(a)->get_start_time() < notes[channel].note_at(b)->get_start_time();
                                     });
        }
    }

    void RaindropProcessedChart::disable_notes_until(const double time) {
        reset_notes();
        for (auto& channel : notes) {
            for (auto& note : channel.handles) {
                if (note.get_start_time() <= time)
                    note.disable();
            }
        }
    }

    void RaindropProcessedChart::reset_notes() {
        for (auto& channel : notes) {
            for (auto& note : channel.handles)
                note.reset();
        }
    }

    RuntimeNote* RaindropProcessedChart::note_at(const uint32_t lane, const RuntimeNoteHandle handle) {
        return lane < notes.size() ? notes[lane].note_at(handle) : nullptr;
    }

    const RuntimeNote* RaindropProcessedChart::note_at(const uint32_t lane, const RuntimeNoteHandle handle) const {
        return lane < notes.size() ? notes[lane].note_at(handle) : nullptr;
    }
}
