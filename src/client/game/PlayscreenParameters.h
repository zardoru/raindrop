#ifndef RAINDROP_PLAYSCREENPARAMETERS_H
#define RAINDROP_PLAYSCREENPARAMETERS_H

// The values here must be consistent with the shaders!
enum EHiddenMode
{
    HM_NONE,
    HM_SUDDEN,
    HM_HIDDEN,
    HM_FLASHLIGHT,
};

struct PlayscreenParameters {
    bool IsSeedSet;
/* rd::TimingType SetupGameSystem(
        std::shared_ptr<rd::ChartInfo> TimingInfo,
        rd::ScoreKeeper* PlayerScoreKeeper);

void SetupGauge(std::shared_ptr<rd::ChartInfo> TimingInfo, rd::ScoreKeeper* PlayerScoreKeeper); */

int Seed;

    void UpdateHidden(double JudgeY);

private:
    struct SHiddenData {
        EHiddenMode		 Mode; // effective mode after upscroll adjustment
        float            Center; // in NDC
        float			 TransitionSize; // in NDC
        float			 CenterSize; // in NDC
    };

    SHiddenData Hidden;

public:

    // == Non Player Options ==
    // If true, assume difficulty is already loaded and is not just metadata
    int Preloaded;

    // Auto mode enabled if true.
    int Auto;

    // Selected starting measure (Preivew mode only)
    int32_t StartMeasure;

    // == Player options ==
    // If true, use upscroll (VSRG only)
    int Upscroll;

    // Fail disabled if true.
    int NoFail;

    // Selected hidden mode (VSRG only)
    int HiddenMode;

    // Music speed
    float Rate;

    // Scroll speed
    double UserSpeedMultiplier;

    // Randomizing mode -> 0 = Disabled, 1 = Per-Lane, 2 = Panic (unimplemented)
    int Random;

    // Gauge type (VSRG only)
    int32_t GaugeType;

    // rd System Type (VSRG only)
    int32_t SystemType;

    // Whether to interpret desired speed
    // as green number
    bool GreenNumber;

    // Whether to enable the use of strictest timing
    bool UseW0;

    int SpeedType;

    /* PlayerChartState* Setup(
            double DesiredDefaultSpeed,
            int SpeedType,
            double Drift,
            std::shared_ptr<VSRG::Difficulty> CurrentDiff);

    std::unique_ptr<rd::VSRG::Mechanics> PrepareMechanicsSet(
            std::shared_ptr<VSRG::Difficulty> CurrentDiff,
            std::shared_ptr<rd::VSRG::ScoreKeeper> PlayerScorekeeper,
            double JudgeY); */

    rd::ScoreType GetScoringType() const;

    int GetHiddenMode() const;
    float GetHiddenCenter() const;
    float GetHiddenTransitionSize() const;
    float GetHiddenCenterSize() const;

    // Last used, or set, seed.
    int GetSeed() const;

    // Use this seed to shuffle.
    void SetSeed(int seed);

    // Unset the seed. Generate a new one.
    void ResetSeed();

    PlayscreenParameters() {
        Upscroll = false;
        Preloaded = false;
        Auto = false;
        NoFail = false;
        GreenNumber = false;
        UseW0 = false;
        HiddenMode = HM_NONE;
        StartMeasure = -1;
        Random = 0;
        Rate = 1;
        GaugeType = rd::LT_AUTO;
        SystemType = rd::TI_NONE;
        UserSpeedMultiplier = 4;
        IsSeedSet = false;
        SpeedType = rd::SPEEDTYPE_DEFAULT;
    }
};

#endif //RAINDROP_PLAYSCREENPARAMETERS_H
