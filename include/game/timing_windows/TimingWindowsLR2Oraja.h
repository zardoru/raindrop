#pragma once


/* LR2Oraja informed timing windows. */
namespace rd {
    class TimingWindowsLR2Oraja : public TimingWindows {
    private:
        std::array<double, JUDGMENT_ARRAY_SIZE> judge_ln_ = {};
        std::array<float, MAX_CHANNELS> hold_head_delta_ = {};
        static void scale_by_def_ex_rank(std::array<double, JUDGMENT_ARRAY_SIZE>& in_out, double scale);
    public:
        void default_setup() override;
        void reset() override;

        /*
         * scale is defexrank (i.e. 100 is 1x)
         * */
        void setup(double strictness, double scale) override;
        ScoreKeeperJudgment get_judgment_for_time_offset(double time_delta, LaneHandle lane, NoteJudgmentPart part) override;

        [[nodiscard]] bool uses_two_judges_per_hold() const override;
    };


}
