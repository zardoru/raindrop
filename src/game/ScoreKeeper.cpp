#include "rmath.h"

#include <game/GameConstants.h>
#include <game/ScoreKeeper.h>

#include <sstream>
#include <iomanip>
#include <numeric>
#include <iostream>

namespace rd {
    ScoreKeeper::~ScoreKeeper() {}

    double ScoreKeeper::accuracy_percent(double var) {
        return double(acc_max_sq - var) / (acc_max_sq - acc_min_sq) * 100;
    }

    void ScoreKeeper::set_acc_min(double ms) {
        acc_min = ms;
        acc_min_sq = ms * ms;
    }

    void ScoreKeeper::set_acc_max(double ms) {
        acc_max = ms;
        acc_max_sq = ms * ms;
    }

    void ScoreKeeper::set_total_objects(int total_objects, int _total_holds) {
        total_score_objects = std::max(total_objects, 1);
        total_holds = _total_holds;
    }

    int ScoreKeeper::get_max_judgable_notes() const {
        if (current_timing_window->uses_two_judges_per_hold())
            return total_score_objects + total_holds;
        else
            return total_score_objects;
    }

    int ScoreKeeper::get_judged_notes() const {
        return judged_notes;
    }


    float ScoreKeeper::get_hit_stdev() const {
        return sqrt(hit_variance / (judged_notes - 1));
    }

    // ms is misleading- since it may very well be beats, but it's fine.
    ScoreKeeperJudgment ScoreKeeper::hit_note(double ms, uint32_t lane, NoteJudgmentPart part) {

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

        auto judge = current_timing_window->get_judgment_for_time_offset(ms, static_cast<uint32_t>(lane), part);
        ScoreKeeperJudgment o2Judge;
        for (auto &timing: timings) {
            /* XXX: this won't really work unless the ms part of this is in beats */
            if (timing.first == TI_O2JAM) {
                ScoreKeeperJudgment o2Judge;

                // we didn't run "getJudgement" for our o2jam state
                if (timing.second != current_timing_window) {
                    o2Judge = timing.second->get_judgment_for_time_offset(ms, lane, part);
                } else  // we did so just reuse it
                    o2Judge = judge;

                o2Judge = static_cast<ScoreKeeperJudgment>(score_o2jam.MutateJudgment(o2Judge));
                timing.second->add_judgment(o2Judge, false);

                // update our temporal judgment if we're using o2jam timing
                if (current_timing_window == &timing_o2jam) {
                    judge = o2Judge;
                }
            } else {
                bool early_miss = ms < -timing.second->get_early_hit_cutoff() && ms >= -timing.second->get_early_threshold();
                if (timing.second == current_timing_window)
                    timing.second->add_judgment(judge, early_miss);
                else {
                    auto myJudge = timing.second->get_judgment_for_time_offset(ms, lane, part);
                    timing.second->add_judgment(myJudge, early_miss);
                }
            }
        }

        // Other methods

        if (judge == SKJ_NONE) // nothing to update
            return judge;

        // we have to miss_note?
        if (judge != SKJ_MISS) {
            // SC, ACC^2 score

            sc_score += clamp(accuracy_percent(ms * ms) / 100, 0.0, 1.0) * 2;
            sc_sc_score += sc_score * clamp(accuracy_percent(ms * ms) / 100, 0.0, 1.0);

            score = double(SCORE_MAX * sc_sc_score) / (get_max_judgable_notes() * (get_max_judgable_notes() + 1));

            // lifebars
            lifebar_hit(abs(ms), judge);

            // scores
            for (auto &scoresys: scores) {
                scoresys.second->Update(judge, uses_w0());
            }
        } else {
            miss_note(false, false, false);
        }

        return judge;
    }

    void ScoreKeeper::lifebar_hit(double ms, rd::ScoreKeeperJudgment judgment) {
        for (auto &gauge: gauges) {
            gauge.second->Update(judgment, true);
        }
    }

    int ScoreKeeper::get_judgment_count(int judgment) const {
        return current_timing_window->get_judgment_count(static_cast<ScoreKeeperJudgment>(judgment));
    }

    bool ScoreKeeper::uses_w0() const {
        return current_timing_window->get_window_skip() == 0;
    }

    void ScoreKeeper::miss_note(bool dont_break_combo, bool early_miss, bool apply_miss) {
        if (apply_miss) {
            for (auto &timing: timings) {
                timing.second->add_judgment(SKJ_MISS, early_miss);
            }

            for (auto &scoresys: scores) {
                scoresys.second->Update(SKJ_MISS, uses_w0());
            }
        }


        if (!early_miss)
            ++judged_notes;

        accuracy = accuracy_percent(total_sqdev / judged_notes);

        for (auto &gauge : gauges) {
            gauge.second->Update(SKJ_MISS, early_miss);
        }

        if (!early_miss && !dont_break_combo) {
            total_sqdev += get_late_miss_cutoff_ms() * get_late_miss_cutoff_ms();

            // TODO: fit this in with the new timing scheme? does it matter?
            // combo = 0;
        }
    }

    double ScoreKeeper::get_judgment_cutoff_ms() {
        return std::accumulate(
                timings.begin(),
                timings.end(),
                0.0,
                [] (double accum, const std::pair<ChartType, TimingWindows*>& wnd) {
                    return std::max(accum, std::max(wnd.second->get_early_threshold(), wnd.second->get_late_threshold()));
                });
    }

    double ScoreKeeper::get_early_miss_cutoff_ms() const {
        return current_timing_window->get_early_threshold();
    }

    double ScoreKeeper::get_early_hit_cutoff_ms() const {
        return current_timing_window->get_early_hit_cutoff();
    }

    double ScoreKeeper::get_late_miss_cutoff_ms() const {
        return current_timing_window->get_late_threshold();
    }

    double ScoreKeeper::get_acc_max() const {
        return acc_max;
    }

    double ScoreKeeper::get_judgment_window(int judgment) {
        return current_timing_window->get_judgment_window(static_cast<ScoreKeeperJudgment>(judgment));
    }

    std::string ScoreKeeper::get_histogram() {
        std::stringstream ss;

        const int histogram_display_width = 15;

        for (int i = 0; i < 255; ++i) {
            int it = (i % histogram_display_width) * (255 / histogram_display_width) +
                     (i / histogram_display_width); // transpose
            ss << std::setw(4) << it - 127 << ": " << std::setw(4) << histogram[it] << " ";
            if (i % histogram_display_width == histogram_display_width - 1)
                ss << "\n";
        }

        return ss.str();
    }

    int ScoreKeeper::get_histogram_point(int point) const {
        int msCount = sizeof(histogram) / sizeof(double) / 2;
        if (abs(point) > msCount) return 0;
        return histogram[point + msCount];
    }

    int ScoreKeeper::get_histogram_point_count() const {
        return sizeof(histogram) / sizeof(double);
    }

    int ScoreKeeper::get_histogram_highest_point() const {
        return std::accumulate(&histogram[0], histogram + get_histogram_point_count(), 1.0,
                               [](double a, double b) -> double {
                                   return std::max(a, b);
                               });
    }

    double ScoreKeeper::get_avg_hit() const {
        return avg_hit;
    }

    /* actual score functions. */

    int ScoreKeeper::get_score(int score_type) const {
        if (scores.find(static_cast<const ScoreType>(score_type)) != scores.end())
            return scores.at(static_cast<ScoreType>(score_type))->GetCurrentScore(get_max_judgable_notes(), uses_w0());

        switch (score_type) {
            case ST_SCORE:
                return int(score);
            case ST_COMBO:
                return current_timing_window->get_combo();
            case ST_MAX_COMBO:
                return current_timing_window->get_max_combo();
            case ST_NOTES_HIT:
                return current_timing_window->get_notes_hit();
            default:
                return 0;
        }
    }

    float ScoreKeeper::get_percent_score(int percent_score_type) const {
        switch (percent_score_type) {
            case PST_RANK:
                if (judged_notes)
                    return double(get_score(ST_RANK)) / double(judged_notes) * 100.0;
                return 100;
            case PST_EX:
                if (judged_notes)
                    return double(get_score(ST_EX)) / double(judged_notes * 2) * 100.0;
                return 100;
            case PST_ACC:
                return accuracy;
            case PST_NH:
                if (judged_notes)
                    return double(current_timing_window->get_notes_hit()) / double(judged_notes) * 100.0;
                return 100;
            case PST_OSU:
                if (judged_notes)
                    return double(get_score(ST_OSUMANIA_ACC)) / 100.0;
                return 100;
            default:
                return 0;
        }
    }

    float ScoreKeeper::get_lifebar_amount(int lifebar_amount_type) const {
        if (gauges.find((LifeType)lifebar_amount_type) != gauges.end())
            return gauges.at((LifeType)lifebar_amount_type)->GetGaugeValue();

        return 0;
   }

    bool ScoreKeeper::is_stage_failed(int lifebar_amount_type) const{
        bool song_ended = judged_notes == get_max_judgable_notes();

        if (gauges.find((LifeType)lifebar_amount_type) != gauges.end())
            return gauges.at((LifeType)lifebar_amount_type)->HasFailed(song_ended);

        return false;
    }

    bool ScoreKeeper::has_delayed_failure(int lifebar_type) {
        if (gauges.find((LifeType)lifebar_type) != gauges.end())
            return gauges[(LifeType)lifebar_type]->HasDelayedFailure();

        return false;
    }

    void ScoreKeeper::fail_stage() {
        judged_notes = get_max_judgable_notes();

        for (auto &scoresys: scores) {
            scoresys.second->Update(SKJ_MISS, uses_w0());
        }
    }

    int ScoreKeeper::get_pacemaker_diff(PacemakerType pacemaker) {
        auto ex_score = get_score(ST_EX);
        auto rank_pts = get_score(ST_RANK);
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

    std::pair<std::string, int> ScoreKeeper::get_auto_pacemaker() {
        auto ex_score = get_score(ST_EX);
        PacemakerType pmt;

        if (ex_score < judged_notes * 2 / 9) pmt = PMT_F;
        else if (ex_score < judged_notes * 5 / 9) pmt = PMT_E;
        else if (ex_score < judged_notes * 7 / 9) pmt = PMT_D;
        else if (ex_score < judged_notes * 9 / 9) pmt = PMT_C;
        else if (ex_score < judged_notes * 11 / 9) pmt = PMT_B;
        else if (ex_score < judged_notes * 13 / 9) pmt = PMT_A;
        else if (ex_score < judged_notes * 15 / 9) pmt = PMT_AA;
        else pmt = PMT_AAA;

        int pacemaker = get_pacemaker_diff(pmt);
        std::stringstream ss;
        ss
                << std::setfill(' ') << std::setw(4) << pacemaker_texts[pmt] << ": ";

        return std::make_pair(ss.str(), pacemaker);
    }

    std::pair<std::string, int> ScoreKeeper::get_auto_rank_pacemaker() {
        auto rank_pts = get_score(ST_RANK);
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

        int pacemaker = get_pacemaker_diff(pmt);

        std::stringstream ss;
        ss
                << std::setfill(' ') << std::setw(4) << pacemaker_texts[pmt] << ": ";

        return std::make_pair(ss.str(), pacemaker);
    }

    void ScoreKeeper::apply_rate_scale(double rate) {
        // TODO: take whatever steps are necessary to not depend on this
        /*for (int i = 0; i <= 6; i++) {
            judgment_time[i] *= rate;
        }

        early_miss_threshold *= rate;
        early_hit_threshold *= rate;
        late_miss_threshold *= rate;
         */
    }

    int ScoreKeeper::get_bm_rank() const {
        return score_ex.GetRank(judged_notes);
    }

    int ScoreKeeper::get_rank() const {
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
    double ScoreKeeper::get_offset_distrust() const {
        // avgHit != 0 is what we're testing
        // don't want this to grow that much with total notes, at all.
        double zscore = abs(get_avg_hit()) / (get_hit_stdev()/* /sqrt(judged_notes) */);
        // we're always off the center, so, sort of scale the result
        double pvalue = (normalCdf(zscore) - 0.5) * 2;

        return pvalue;
    }

    void ScoreKeeper::use_lr2_timing() {
        current_timing_window = &timing_lr2;
    }

    void ScoreKeeper::set_osu_hp(float hp) {
        gauge_osumania.Setup(0, 0, hp);
    }

    double ScoreKeeper::get_ln_tick_interval() {
        return current_timing_window->get_tick_interval();
    }

    void ScoreKeeper::tick_ln(int ticks) {
        for (auto i = 0; i < ticks; i++) {
            for (auto &timing : timings) {
                timing.second->add_judgment(SKJ_TICK, false);
            }

            for (auto &gauge : gauges) {
                gauge.second->Update(SKJ_TICK, false, 0);
            }
        }
    }

    bool ScoreKeeper::is_o2jam() const {
        return use_o2jam;
    }

    int ScoreKeeper::get_cool_combo() const {
        return score_o2jam.GetCoolCombo();
    }

    uint8_t ScoreKeeper::get_pills() const {
        return score_o2jam.GetPills();
    }

} // namespace rd
