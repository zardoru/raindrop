#include "rmath.h"

#include <game/GameConstants.h>
#include <game/ScoreKeeper7K.h>

#include <sstream>
#include <iomanip>
#include <numeric>
#include <iostream>
#include <TextAndFileUtil.h>

namespace rd {
    ScoreKeeper::~ScoreKeeper() {}

    double ScoreKeeper::accuracy_percent(double var) {
        return double(ACC_MAX_SQ - var) / (ACC_MAX_SQ - ACC_MIN_SQ) * 100;
    }

    void ScoreKeeper::setAccMin(double ms) {
        ACC_MIN = ms;
        ACC_MIN_SQ = ms * ms;
    }

    void ScoreKeeper::setAccMax(double ms) {
        ACC_MAX = ms;
        ACC_MAX_SQ = ms * ms;
    }

    void ScoreKeeper::setTotalObjects(int total_objects, int _total_holds) {
        total_score_objects = std::max(total_objects, 1);
        total_holds = _total_holds;
    }

    int ScoreKeeper::getMaxJudgableNotes() const {
        if (CurrentTimingWindow->UsesTwoJudgesPerHold())
            return total_score_objects + total_holds;
        else
            return total_score_objects;
    }

    int ScoreKeeper::getJudgedNotes() const {
        return judged_notes;
    }


    float ScoreKeeper::getHitStDev() const {
        return sqrt(hit_variance / (judged_notes - 1));
    }

    // ms is misleading- since it may very well be beats, but it's fine.
    ScoreKeeperJudgment ScoreKeeper::hitNote(double ms, uint32_t lane, NoteJudgmentPart part) {

        // online variance and average hit
        ++judged_notes;
        float delta = ms - avg_hit;
        avg_hit += delta / judged_notes;

        hit_variance += delta * (ms - avg_hit);


        auto rounded = round(ms);
        if (use_o2jam) {
            auto dist = ms / O2_WINDOW * 128;
            if (std::abs(rounded) < 128) {
                ++histogram[static_cast<int>(rounded) + 127];
            }
        } else {
            if (std::abs(rounded) < 128) {
                ++histogram[static_cast<int>(rounded) + 127];
            }
        }


       // accuracy score
        if (use_o2jam)
            total_sqdev += ms * ms / pow(O2_WINDOW, 2);
        else
            total_sqdev += ms * ms;

        accuracy = accuracy_percent(total_sqdev / judged_notes);

        // judgments
        /* add judgment, handle combo etc.. */

        auto judge = CurrentTimingWindow->GetJudgmentForTimeOffset(ms, static_cast<uint32_t>(lane), part);
        ScoreKeeperJudgment o2Judge;
        for (auto &timing: Timings) {
            /* XXX: this won't really work unless the ms part of this is in beats */
            if (timing.first == TI_O2JAM) {
                ScoreKeeperJudgment o2Judge;

                // we didn't run "getJudgement" for our o2jam state
                if (timing.second != CurrentTimingWindow) {
                    o2Judge = timing.second->GetJudgmentForTimeOffset(ms, lane, part);
                } else  // we did so just reuse it
                    o2Judge = judge;

                o2Judge = static_cast<ScoreKeeperJudgment>(score_o2jam.MutateJudgment(o2Judge));
                timing.second->AddJudgment(o2Judge, false);

                // update our temporal judgment if we're using o2jam timing
                if (CurrentTimingWindow == &timing_o2jam) {
                    judge = o2Judge;
                }
            } else {
                bool early_miss = ms < -timing.second->GetEarlyHitCutoff() && ms >= -timing.second->GetEarlyThreshold();
                if (timing.second == CurrentTimingWindow)
                    timing.second->AddJudgment(judge, early_miss);
                else {
                    auto myJudge = timing.second->GetJudgmentForTimeOffset(ms, lane, part);
                    timing.second->AddJudgment(myJudge, early_miss);
                }
            }
        }

        // Other methods

        if (judge == SKJ_NONE) // nothing to update
            return judge;

        // we have to missNote?
        if (judge != SKJ_MISS) {
            // SC, ACC^2 score

            sc_score += Clamp(accuracy_percent(ms * ms) / 100, 0.0, 1.0) * 2;
            sc_sc_score += sc_score * Clamp(accuracy_percent(ms * ms) / 100, 0.0, 1.0);

            score = double(SCORE_MAX * sc_sc_score) / (getMaxJudgableNotes() * (getMaxJudgableNotes() + 1));

            // lifebars
            lifebarHit(abs(ms), judge);

            // scores
            for (auto &scoresys: Scores) {
                scoresys.second->Update(judge, usesW0());
            }
        } else {
            missNote(false, false, false);
        }

        return judge;
    }

    void ScoreKeeper::lifebarHit(double ms, rd::ScoreKeeperJudgment judgment) {
        for (auto &gauge: Gauges) {
            gauge.second->Update(judgment, true);
        }
    }

    int ScoreKeeper::getJudgmentCount(int judgment) {
        return CurrentTimingWindow->GetJudgmentCount(static_cast<ScoreKeeperJudgment>(judgment));
    }

    bool ScoreKeeper::usesW0() const {
        return CurrentTimingWindow->GetWindowSkip() == 0;
    }

    void ScoreKeeper::missNote(bool dont_break_combo, bool early_miss, bool apply_miss) {
        if (apply_miss) {
            for (auto &timing: Timings) {
                timing.second->AddJudgment(SKJ_MISS, false);
            }

            for (auto &scoresys: Scores) {
                scoresys.second->Update(SKJ_MISS, usesW0());
            }
        }


        if (!early_miss)
            ++judged_notes;

        accuracy = accuracy_percent(total_sqdev / judged_notes);

        for (auto &gauge : Gauges) {
            gauge.second->Update(SKJ_MISS, early_miss);
        }

        if (!early_miss && !dont_break_combo) {
            total_sqdev += getLateMissCutoffMS() * getLateMissCutoffMS();

            // TODO: fit this in with the new timing scheme? does it matter?
            // combo = 0;
        }
    }

    double ScoreKeeper::getJudgmentCutoffMS() {
        return std::accumulate(
                Timings.begin(),
                Timings.end(),
                0.0,
                [] (double accum, const std::pair<ChartType, TimingWindows*>& wnd) {
                    return std::max(accum, std::max(wnd.second->GetEarlyThreshold(), wnd.second->GetLateThreshold()));
                });
    }

    double ScoreKeeper::getEarlyMissCutoffMS() const {
        return CurrentTimingWindow->GetEarlyThreshold();
    }

    double ScoreKeeper::getEarlyHitCutoffMS() const {
        return CurrentTimingWindow->GetEarlyHitCutoff();
    }

    double ScoreKeeper::getLateMissCutoffMS() const {
        return CurrentTimingWindow->GetLateThreshold();
    }

    double ScoreKeeper::getAccMax() const {
        return ACC_MAX;
    }

    double ScoreKeeper::getJudgmentWindow(int judgment) {
        return CurrentTimingWindow->GetJudgmentWindow(static_cast<ScoreKeeperJudgment>(judgment));
    }

    std::string ScoreKeeper::getHistogram() {
        std::stringstream ss;

        const int HISTOGRAM_DISPLAY_WIDTH = 15;

        for (int i = 0; i < 255; ++i) {
            int it = (i % HISTOGRAM_DISPLAY_WIDTH) * (255 / HISTOGRAM_DISPLAY_WIDTH) +
                     (i / HISTOGRAM_DISPLAY_WIDTH); // transpose
            ss << std::setw(4) << it - 127 << ": " << std::setw(4) << histogram[it] << " ";
            if (i % HISTOGRAM_DISPLAY_WIDTH == HISTOGRAM_DISPLAY_WIDTH - 1)
                ss << "\n";
        }

        return ss.str();
    }

    int ScoreKeeper::getHistogramPoint(int point) const {
        int msCount = sizeof(histogram) / sizeof(double) / 2;
        if (abs(point) > msCount) return 0;
        return histogram[point + msCount];
    }

    int ScoreKeeper::getHistogramPointCount() const {
        return sizeof(histogram) / sizeof(double);
    }

    int ScoreKeeper::getHistogramHighestPoint() const {
        return std::accumulate(&histogram[0], histogram + getHistogramPointCount(), 1.0,
                               [](double a, double b) -> double {
                                   return std::max(a, b);
                               });
    }

    double ScoreKeeper::getAvgHit() const {
        return avg_hit;
    }

    /* actual score functions. */

    int ScoreKeeper::getScore(int score_type) const {
        if (Scores.find(static_cast<const ScoreType>(score_type)) != Scores.end())
            return Scores.at(static_cast<ScoreType>(score_type))->GetCurrentScore(getMaxJudgableNotes(), usesW0());

        switch (score_type) {
            case ST_SCORE:
                return int(score);
            case ST_COMBO:
                return CurrentTimingWindow->GetCombo();
            case ST_MAX_COMBO:
                return CurrentTimingWindow->GetMaxCombo();
            case ST_NOTES_HIT:
                return CurrentTimingWindow->GetNotesHit();
            default:
                return 0;
        }
    }

    float ScoreKeeper::getPercentScore(int percent_score_type) const {
        switch (percent_score_type) {
            case PST_RANK:
                if (judged_notes)
                    return double(getScore(ST_RANK)) / double(judged_notes) * 100.0;
                return 100;
            case PST_EX:
                if (judged_notes)
                    return double(getScore(ST_EX)) / double(judged_notes * 2) * 100.0;
                return 100;
            case PST_ACC:
                return accuracy;
            case PST_NH:
                if (judged_notes)
                    return double(CurrentTimingWindow->GetNotesHit()) / double(judged_notes) * 100.0;
                return 100;
            case PST_OSU:
                if (judged_notes)
                    return double(getScore(ST_OSUMANIA_ACC)) / 100.0;
                return 100;
            default:
                return 0;
        }
    }

    float ScoreKeeper::getLifebarAmount(int lifebar_amount_type) {
        if (Gauges.find((LifeType)lifebar_amount_type) != Gauges.end())
            return Gauges[(LifeType)lifebar_amount_type]->GetGaugeValue();

        return 0;
   }

    bool ScoreKeeper::isStageFailed(int lifebar_amount_type) {
        bool song_ended = judged_notes == getMaxJudgableNotes();

        if (Gauges.find((LifeType)lifebar_amount_type) != Gauges.end())
            return Gauges[(LifeType)lifebar_amount_type]->HasFailed(song_ended);

        return false;
    }

    bool ScoreKeeper::hasDelayedFailure(int lifebar_type) {
        if (Gauges.find((LifeType)lifebar_type) != Gauges.end())
            return Gauges[(LifeType)lifebar_type]->HasDelayedFailure();

        return false;
    }

    void ScoreKeeper::failStage() {
        judged_notes = getMaxJudgableNotes();

        for (auto &scoresys: Scores) {
            scoresys.second->Update(SKJ_MISS, usesW0());
        }
    }

    int ScoreKeeper::getPacemakerDiff(PacemakerType pacemaker) {
        auto ex_score = getScore(ST_EX);
        auto rank_pts = getScore(ST_RANK);
        switch (pacemaker) {
            case PMT_F:
                return ex_score - (judged_notes * 2 / 9 + (judged_notes * 2 % 9 != 0));
            case PMT_E:
                return ex_score - (judged_notes * 4 / 9 + (judged_notes * 4 % 9 != 0));
            case PMT_D:
                return ex_score - (judged_notes * 6 / 9 + (judged_notes * 6 % 9 != 0));
            case PMT_C:
                return ex_score - (judged_notes * 8 / 9 + (judged_notes * 8 % 9 != 0));
            case PMT_B:
                return ex_score - (judged_notes * 10 / 9 + (judged_notes * 10 % 9 != 0));
            case PMT_A:
                return ex_score - (judged_notes * 12 / 9 + (judged_notes * 12 % 9 != 0));
            case PMT_AA:
                return ex_score - (judged_notes * 14 / 9 + (judged_notes * 14 % 9 != 0));
            case PMT_AAA:
                return ex_score - (judged_notes * 16 / 9 + (judged_notes * 16 % 9 != 0));

            case PMT_50EX:
                return ex_score - (judged_notes);
            case PMT_75EX:
                return ex_score - (judged_notes * 75 / 50);
            case PMT_85EX:
                return ex_score - (judged_notes * 85 / 50);

            case PMT_RANK_ZERO:
                return rank_pts - (judged_notes * 100 / 100);
            case PMT_RANK_P1:
                return rank_pts - (judged_notes * 120 / 100 + (judged_notes * 120 % 100 != 0));
            case PMT_RANK_P2:
                return rank_pts - (judged_notes * 140 / 100 + (judged_notes * 140 % 100 != 0));
            case PMT_RANK_P3:
                return rank_pts - (judged_notes * 160 / 100 + (judged_notes * 160 % 100 != 0));
            case PMT_RANK_P4:
                return rank_pts - (judged_notes * 180 / 100 + (judged_notes * 180 % 100 != 0));
            case PMT_RANK_P5:
                return rank_pts - (judged_notes * 200 / 100);
            case PMT_RANK_P6:
                return rank_pts - (judged_notes * 220 / 100 + (judged_notes * 220 % 100 != 0));
            case PMT_RANK_P7:
                return rank_pts - (judged_notes * 240 / 100 + (judged_notes * 240 % 100 != 0));
            case PMT_RANK_P8:
                return rank_pts - (judged_notes * 260 / 100 + (judged_notes * 260 % 100 != 0));
            case PMT_RANK_P9:
                return rank_pts - (judged_notes * 280 / 100 + (judged_notes * 280 % 100 != 0));

            default:
                break;
        }

        return 0;
    }

    std::pair<std::string, int> ScoreKeeper::getAutoPacemaker() {
        auto ex_score = getScore(ST_EX);
        PacemakerType pmt;

        if (ex_score < judged_notes * 2 / 9) pmt = PMT_F;
        else if (ex_score < judged_notes * 5 / 9) pmt = PMT_E;
        else if (ex_score < judged_notes * 7 / 9) pmt = PMT_D;
        else if (ex_score < judged_notes * 9 / 9) pmt = PMT_C;
        else if (ex_score < judged_notes * 11 / 9) pmt = PMT_B;
        else if (ex_score < judged_notes * 13 / 9) pmt = PMT_A;
        else if (ex_score < judged_notes * 15 / 9) pmt = PMT_AA;
        else pmt = PMT_AAA;

        int pacemaker = getPacemakerDiff(pmt);
        std::stringstream ss;
        ss
                << std::setfill(' ') << std::setw(4) << pacemaker_texts[pmt] << ": ";

        return std::make_pair(ss.str(), pacemaker);
    }

    std::pair<std::string, int> ScoreKeeper::getAutoRankPacemaker() {
        auto rank_pts = getScore(ST_RANK);
        PacemakerType pmt;
        if (rank_pts < judged_notes * 110 / 100) pmt = PMT_RANK_ZERO;
        else if (rank_pts < judged_notes * 130 / 100) pmt = PMT_RANK_P1;
        else if (rank_pts < judged_notes * 150 / 100) pmt = PMT_RANK_P2;
        else if (rank_pts < judged_notes * 170 / 100) pmt = PMT_RANK_P3;
        else if (rank_pts < judged_notes * 190 / 100) pmt = PMT_RANK_P4;
        else if (rank_pts < judged_notes * 210 / 100) pmt = PMT_RANK_P5;
        else if (rank_pts < judged_notes * 230 / 100) pmt = PMT_RANK_P6;
        else if (rank_pts < judged_notes * 250 / 100) pmt = PMT_RANK_P7;
        else if (rank_pts < judged_notes * 270 / 100) pmt = PMT_RANK_P8;
        else pmt = PMT_RANK_P9;

        int pacemaker = getPacemakerDiff(pmt);

        std::stringstream ss;
        ss
                << std::setfill(' ') << std::setw(4) << pacemaker_texts[pmt] << ": ";

        return std::make_pair(ss.str(), pacemaker);
    }

    void ScoreKeeper::applyRateScale(double rate) {
        // TODO: take whatever steps are necessary to not depend on this
        /*for (int i = 0; i <= 6; i++) {
            judgment_time[i] *= rate;
        }

        early_miss_threshold *= rate;
        early_hit_threshold *= rate;
        late_miss_threshold *= rate;
         */
    }

    int ScoreKeeper::getBMRank() const {
        return score_ex.GetRank(judged_notes);
    }

    int ScoreKeeper::getRank() const {
        return score_rank.GetRank();
    }

    double normalCdf(double x) {
        return 0.5 + 0.5 * erf(x / sqrt(2));
    }

    // bisection method erf. inverse
    //double cdf_percentile(double val) {
    //	const auto error = 1e-4; // 4 digits is good enough lol
    //	double lo = -1000;
    //	double hi = 1000;

    //	double center = (hi + lo) / 2;
    //	while (abs(normalCdf(center) - val) > error) {
    //		double result = normalCdf(center);

    //		if (result < val)
    //			lo = center;
    //		if (result > val)
    //			hi = center;

    //		center = (hi + lo) / 2;
    //	}

    //	return center;
    //}

    // return p-value
    double ScoreKeeper::getOffsetDistrust() const {
        // avgHit != 0 is what we're testing
        // don't want this to grow that much with total notes, at all.
        double zscore = abs(getAvgHit()) / (getHitStDev()/* /sqrt(judged_notes) */);
        // we're always off the center, so, sort of scale the result
        double pvalue = (normalCdf(zscore) - 0.5) * 2;

        return pvalue;
    }

    void ScoreKeeper::useLR2Timing() {
        CurrentTimingWindow = &timing_lr2;
    }

    void ScoreKeeper::setOsuHP(float hp) {
        gauge_osumania.Setup(0, 0, hp);
    }

    double ScoreKeeper::getLNTickInterval() {
        return CurrentTimingWindow->GetTickInterval();
    }

    void ScoreKeeper::tickLN(int ticks) {
        for (auto i = 0; i < ticks; i++) {
            for (auto &timing : Timings) {
                timing.second->AddJudgment(SKJ_TICK, false);
            }

            for (auto &gauge : Gauges) {
                gauge.second->Update(SKJ_TICK, false, 0);
            }
        }
    }

    bool ScoreKeeper::usesO2() const {
        return use_o2jam;
    }

    int ScoreKeeper::getCoolCombo() const {
        return score_o2jam.GetCoolCombo();
    }

    uint8_t ScoreKeeper::getPills() const {
        return score_o2jam.GetPills();
    }

} // namespace rd