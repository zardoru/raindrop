
#pragma once
#include "game/GameConstants.h"

// The values here must be consistent with the shaders!
enum EHiddenMode
{
    HM_NONE,
    HM_SUDDEN,
    HM_HIDDEN,
    HM_FLASHLIGHT,
};

struct PlayscreenParameters {
    bool is_seed_set;
int seed;

    void update_hidden(double judge_y);

private:
    struct SHiddenData {
        EHiddenMode		 mode; // effective mode after upscroll adjustment
        float            center; // in NDC
        float			 transition_size; // in NDC
        float			 center_size; // in NDC
    };

    SHiddenData hidden_;

public:

    // == Non Player Options ==
    // If true, assume difficulty is already loaded and is not just metadata
    int preloaded;

    // Auto mode enabled if true.
    int Auto;

    // Selected starting measure (Preivew mode only)
    int32_t start_measure;

    // == Player options ==
    // If true, use upscroll (VSRG only)
    int upscroll;

    // Fail disabled if true.
    int no_fail;

    // Selected hidden mode (VSRG only)
    int hidden_mode;

    // Music speed
    float rate;

    // Scroll speed
    double user_speed_multiplier;

    // Randomizing mode -> 0 = Disabled, 1 = Per-Lane, 2 = Panic (unimplemented)
    int random;

    // Gauge type (VSRG only)
    int32_t gauge_type;

    // rd System Type (VSRG only)
    int32_t system_type;

    // Whether to interpret desired speed
    // as green number
    bool green_number;

    // Whether to enable the use of strictest timing
    bool use_w0;

    int speed_type;

    rd::ScoreType get_scoring_type() const;

    int get_hidden_mode() const;
    float get_hidden_center() const;
    float get_hidden_transition_size() const;
    float get_hidden_center_size() const;

    // Last used, or set, seed.
    int get_seed() const;

    // Use this seed to shuffle.
    void set_seed(int seed);

    // Unset the seed. Generate a new one.
    void reset_seed();

    PlayscreenParameters() {
        upscroll = false;
        preloaded = false;
        Auto = false;
        no_fail = false;
        green_number = false;
        use_w0 = false;
        hidden_mode = HM_NONE;
        start_measure = -1;
        random = 0;
        rate = 1;
        gauge_type = rd::LT_AUTO;
        system_type = rd::TI_NONE;
        user_speed_multiplier = 4;
        is_seed_set = false;
        speed_type = rd::SPEEDTYPE_DEFAULT;
    }
};
