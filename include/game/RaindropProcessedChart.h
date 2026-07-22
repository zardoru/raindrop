#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <ProcessedChart.h>
#include <game/GameConstants.h>

namespace rd {
    struct RuntimeNoteStorage;

    class RuntimeNote {
        RuntimeNoteStorage* storage;
        uint32_t index;

    public:
        RuntimeNote();
        RuntimeNote(RuntimeNoteStorage* storage, uint32_t index);

        double& get_data_start_time();
        double& get_data_end_time();
        uint32_t& get_data_sound();
        uint8_t get_data_note_kind() const;
        uint8_t get_data_fraction_kind() const;

        void assign_from(otoworm::TrackNote& note);
        void assign_position(double position, double end_position = 0);
        void assign_fraction(double fraction);
        void hit();
        void add_time(double time);
        void disable();
        void disable_head();
        float get_vertical() const;
        float get_vertical_hold() const;
        bool is_hold() const;
        bool is_enabled() const;
        bool is_head_enabled() const;
        bool was_hit() const;
        bool is_judgable() const;
        bool is_visible() const;
        uint32_t get_sound() const;
        uint32_t get_tail_sound() const;
        double get_end_time() const;
        double get_start_time() const;
        int get_frac_kind() const;
        float get_hold_size() const;
        float get_hold_end_vertical() const;
        void fail_hit();
        bool failed_hit() const;
        void make_invisible();
        void remove_sound();
        void reset();
    };

    struct RuntimeNoteStorage {
        std::vector<double> start_time;
        std::vector<double> end_time;
        std::vector<double> vertical;
        std::vector<double> hold_end_vertical;
        std::vector<uint32_t> sound;
        std::vector<uint32_t> tail_sound;
        std::vector<uint8_t> note_kind;
        std::vector<uint8_t> fraction_kind;
        std::vector<uint8_t> flags;
        std::vector<RuntimeNote> handles;

        void clear();
        uint32_t size() const;
        RuntimeNote& push_note(otoworm::TrackNote& note);
        void rebuild_handles();
        RuntimeNote* note_at(uint32_t handle);
        const RuntimeNote* note_at(uint32_t handle) const;
        void swap_notes(uint32_t a, uint32_t b);
    };

    using RuntimeNoteHandle = uint32_t;
    using RuntimeNoteHandleList = std::vector<RuntimeNoteHandle>;

    struct RaindropProcessedChart : otoworm::ProcessedChart {
        std::array<RuntimeNoteStorage, otoworm::max_channels> notes;
        std::array<RuntimeNoteHandleList, otoworm::max_channels> notes_vertically_ordered;
        std::array<RuntimeNoteHandleList, otoworm::max_channels> notes_time_ordered;
        std::vector<double> barlines;

        explicit RaindropProcessedChart(double wait_time = DEFAULT_WAIT_TIME);

        std::vector<double> generate_measure_lines() const;
        void prepare_ordered_notes();
        void disable_notes_until(double time);
        void reset_notes();
        RuntimeNote* note_at(uint32_t lane, RuntimeNoteHandle handle);
        const RuntimeNote* note_at(uint32_t lane, RuntimeNoteHandle handle) const;

        static RaindropProcessedChart from(otoworm::Chart* chart, double speed = 0);
    };
}
