#pragma once


/* LR2Oraja informed timing windows. */
namespace rd {
    class TimingWindowsLR2Oraja : public TimingWindows {
    private:
        std::array<double, JUDGMENT_ARRAY_SIZE> judgeLN;
        std::array<float, MAX_CHANNELS> hold_head_delta;
        static void ScaleByDefExRank(std::array<double, JUDGMENT_ARRAY_SIZE>& in_out, double scale);
    public:
        void DefaultSetup() override;
        void Reset() override;

        /*
         * scale is defexrank (i.e. 100 is 1x)
         * */
        void Setup(double strictness, double scale) override;
        ScoreKeeperJudgment GetJudgmentForTimeOffset(double time_delta, uint32_t lane, NoteJudgmentPart part) override;

        [[nodiscard]] bool UsesTwoJudgesPerHold() const override;
    };


}