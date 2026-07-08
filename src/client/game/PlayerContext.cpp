#include <rmath.h>
#include <queue>
#include <game/Song.h>
#include <game/ScoreKeeper.h>
#include <game/RaindropProcessedChart.h>
#include <game/VSRGMechanics.h>
#include <game/NoteTransformations.h>


#include <glm.h>
#include <text_and_file_util.h>

#include "LuaManager.h"
#include <LuaBridge/LuaBridge.h>

#include "PlayscreenParameters.h"
#include "PlayerContext.h"

#include <algorithm>

#include "Logging.h"
#include "Line.h"

#include "GameWindow.h"

#include "Transformation.h"
#include "Rendering.h"
#include "Sprite.h"
#include "Font.h"
#include "BitmapFont.h"
#include "Shader.h"

#include "../structure/Configuration.h"

#include "Noteskin.h"
#include "Replay.h"

BitmapFont *fnt = nullptr;
CfgVar DebugNoteRendering("NoteRender", "Debug");
SkinMetric PLAYFIELD_SIZE("PlayfieldSize");
SkinMetric UNITS_PER_MEASURE("UnitsPerMeasure");

using namespace rd;

namespace {
    ChartType ToRdChartType(const otoworm::ChartClass chart_class) {
        switch (chart_class) {
            case otoworm::CC_BMS: return TI_BMS;
            case otoworm::CC_OSUMANIA: return TI_OSUMANIA;
            case otoworm::CC_O2JAM: return TI_O2JAM;
            case otoworm::CC_STEPMANIA: return TI_STEPMANIA;
            case otoworm::CC_NULL: return TI_NONE;
        }

        return TI_NONE;
    }

    ChartType GetChartType(const std::shared_ptr<otoworm::ChartInfo> &timing_info) {
        return timing_info ? ToRdChartType(timing_info->get_class()) : TI_NONE;
    }

    std::shared_ptr<otoworm::ChartInfo> GetOtoTimingInfo(const std::shared_ptr<otoworm::Chart> &chart) {
        if (!chart || !chart->transient)
            return nullptr;

        return chart->transient->specialized_info;
    }

    otoworm::ChartTransient *GetOtoTransient(const std::shared_ptr<otoworm::Chart> &chart) {
        if (!chart)
            return nullptr;

        return chart->transient.get();
    }
}

PlayerContext::PlayerContext(const int pn, PlayscreenParameters p) : chart_state_(DEFAULT_WAIT_TIME) {
    noteskin_ = std::make_unique<Noteskin>(this);
    replay_data_ = std::make_shared<Replay>();
    player_number_ = pn;
    drift_ = 0;
    judge_offset_ = 0;

    judge_notes_ = true;

    player_score_keeper_ = std::make_shared<ScoreKeeper>();
    parameters_ = p;

    gear_state_ = {};

    if (!fnt && DebugNoteRendering) {
        fnt = new BitmapFont();
        fnt->load_skin_font_image("font.tga", Vec2(6, 15), Vec2(8, 16), Vec2(6, 15), 0);
    }

    barline_ = nullptr;
}

PlayerContext::~PlayerContext() = default;

void PlayerContext::init() const {
    noteskin_->init_noteskin(chart_state_.has_turntable, current_chart_->channels);
}

void PlayerContext::validate() {
    noteskin_->validate();
    ms_display_margin_ = (Configuration::GetSkinConfigf("HitErrorDisplayLimiter"));


    if (!bind_keys_to_lanes(chart_state_.has_turntable))
        if (!bind_keys_to_lanes(!chart_state_.has_turntable))
            Log::LogPrintf("Couldn't get valid bindings for current key count %d.\n", get_channel_count());

    if (noteskin_->IsBarlineEnabled())
        barline_ = std::make_unique<Line>();
}


const RaindropProcessedChart &PlayerContext::get_chart_state() {
    return chart_state_;
}

TimingType setup_game_system(
    const PlayscreenParameters &param,
    const std::shared_ptr<otoworm::ChartInfo> &timing_info,
    ScoreKeeper *player_score_keeper) {
    TimingType UsedTimingType = TT_TIME;
    const auto chart_type = GetChartType(timing_info);

    if (param.SystemType == TI_BMS || param.SystemType == TI_RDAC || param.SystemType == TI_LR2) {
        UsedTimingType = TT_TIME;
        if (chart_type == TI_BMS) {
            if (const auto info = dynamic_cast<otoworm::BMSChartInfo *>(timing_info.get());
                !info->percentual_judgerank)
                player_score_keeper->setJudgeRank(info->judge_rank);
            else
                player_score_keeper->setJudgeScale(info->judge_rank / 100.0);
        } else {
            player_score_keeper->setJudgeRank(2);
        }

        if (param.SystemType == TI_LR2) {
            player_score_keeper->useLR2Timing();
        }
    } else if (param.SystemType == TI_O2JAM) {
        UsedTimingType = TT_BEATS;
        player_score_keeper->setJudgeRank(-100);
    } else if (param.SystemType == TI_OSUMANIA) {
        UsedTimingType = TT_TIME;
        if (chart_type == TI_OSUMANIA) {
            const auto info = dynamic_cast<otoworm::OsumaniaChartInfo *>(timing_info.get());
            player_score_keeper->setODWindows(info->overall_difficulty);
        } else player_score_keeper->setODWindows(7);
    } else if (param.SystemType == TI_STEPMANIA) {
        UsedTimingType = TT_TIME;
        player_score_keeper->setSMJ4Windows();
    } else if (param.SystemType == TI_RAINDROP) {
        // LifebarType = LT_STEPMANIA;
    } else {
        Log::LogPrintf("Warning: unknown SystemType %d\n", param.SystemType);
    }

    player_score_keeper->applyRateScale(param.Rate);
    return UsedTimingType;
}


auto setup(
    PlayscreenParameters &param,
    double desired_default_speed,
    int type,
    double drift,
    const std::shared_ptr<otoworm::Chart> &current_chart
) -> RaindropProcessedChart * {
    /*
    * 		There are four kinds of speed modifiers:
    * 		-CMod (Keep speed the same through the song, equal to a constant)
    * 		-MMod (Find highest speed and set multiplier to such that the highest speed is equal to a constant)
    *		-First (Find the first speed in the chart, and set multiplier to such that the first speed is equal to a constant)
    *		-Mode (Find the speed that lasts the most in the chart, set multiplier based on that)
    *
    *		The calculations are done ahead, and while SpeedConstant = 0 either MMod or first are assumed
    *		but only if there's a constant specified by the user.
    */

    RaindropProcessedChart chart_state(DEFAULT_WAIT_TIME);

    if (desired_default_speed != 0) {
        desired_default_speed /= param.Rate;

        if (type == SPEEDTYPE_CMOD) // cmod
        {
            param.UserSpeedMultiplier = 1;

            double spd = param.GreenNumber ? 1000 : desired_default_speed;

            chart_state = RaindropProcessedChart::from(current_chart.get(), spd);
        } else
            chart_state = RaindropProcessedChart::from(current_chart.get());

        // if GN is true, Default Speed = GN!
        // Convert GN to speed.
        if (param.GreenNumber) {
            // v0 is normal speed
            // (green number speed * green number time) / (normal speed * normal time)
            // equals
            // playfield distance / note distance

            // simplifying that gets you dn / tn lol
            double new_speed = PLAYFIELD_SIZE / (desired_default_speed / 1000);
            desired_default_speed = new_speed;

            if (type == SPEEDTYPE_CMOD) {
                param.UserSpeedMultiplier = desired_default_speed / 1000;
            }
        }

        if (type == SPEEDTYPE_MMOD) // mmod
        {
            double speed_max = 0; // Find the highest speed
            for (auto i: chart_state.speeds) {
                speed_max = std::max(speed_max, abs(i.value));
            }

            double ratio = desired_default_speed / speed_max; // How much above or below are we from the maximum speed?
            param.UserSpeedMultiplier = ratio;
        } else if (type == SPEEDTYPE_FIRST) // First speed.
        {
            double desired_multiplier = desired_default_speed / chart_state.speeds[0].value;
            param.UserSpeedMultiplier = desired_multiplier;
        } else if (type == SPEEDTYPE_MODE) // Most lasting speed.
        {
            std::map<double, double> freq;
            for (auto i = chart_state.speeds.begin(); i != chart_state.speeds.end(); i++) {
                if (i + 1 != chart_state.speeds.end()) {
                    freq[i->value] += (i + 1)->time - i->time;
                } else freq[i->value] += abs(current_chart->duration - i->time);
            }
            auto max_duration = std::numeric_limits<decltype(freq)::value_type::second_type>::lowest();
            decltype(freq)::key_type speed = 1000.f;
            for (auto [_value, duration]: freq) {
                if (duration > max_duration) {
                    max_duration = duration;
                    speed = _value;
                }
            }

            param.UserSpeedMultiplier = desired_default_speed / (speed * UNITS_PER_MEASURE);
        } else if (type == SPEEDTYPE_MULTIPLIER) // target speed is just a multiplier
        {
            param.UserSpeedMultiplier = desired_default_speed;
        } else if (type != SPEEDTYPE_CMOD) // other cases
        {
            double bpsd = 4.0 / (chart_state.bps[0].value);
            double Speed = (UNITS_PER_MEASURE / bpsd);
            double DesiredMultiplier = desired_default_speed / Speed;

            param.UserSpeedMultiplier = DesiredMultiplier;
        }
    } else
        chart_state = RaindropProcessedChart::from(current_chart.get(), drift);

    if (param.Random) {
        if (!param.IsSeedSet)
            param.set_seed(time(nullptr));


        NoteTransform::Randomize(
            chart_state.notes,
            current_chart->channels,
            chart_state.has_turntable,
            param.Seed
        );
    }


    // Sinisterrr/fully negative charts fix.
    param.UserSpeedMultiplier = abs(param.UserSpeedMultiplier);
    return new RaindropProcessedChart(chart_state);
}

void setup_gauge(
    PlayscreenParameters &param,
    const std::shared_ptr<otoworm::ChartInfo> &timing_info,
    ScoreKeeper *scorekeeper) {
    const auto chart_type = GetChartType(timing_info);
    if (param.GaugeType == LT_AUTO) {
        switch (param.SystemType) {
            case TI_BMS:
            case TI_RAINDROP:
            case TI_RDAC:
                param.GaugeType = LT_GROOVE;
                break;
            case TI_LR2: // LR2's groove gauge
                param.GaugeType = LT_LR2_NORMAL;
                break;
            case TI_O2JAM:
                param.GaugeType = LT_O2JAM;
                break;
            case TI_OSUMANIA:
                param.GaugeType = LT_OSUMANIA;
                break;
            case TI_STEPMANIA:
                param.GaugeType = LT_STEPMANIA;
                break;
            default:
                throw std::runtime_error("Invalid requested system.");
        }
    }

    switch (param.GaugeType) {
        case LT_STEPMANIA:
            // LifebarType = LT_STEPMANIA; // Needs no setup.
            break;
        case LT_OSUMANIA:
            if (chart_type == TI_OSUMANIA) {
                const auto info = dynamic_cast<otoworm::OsumaniaChartInfo *>(timing_info.get());
                scorekeeper->setOsuHP(info->hp);
            }

        case LT_O2JAM:
            if (chart_type == TI_O2JAM) {
                const auto info = dynamic_cast<otoworm::O2JamChartInfo *>(timing_info.get());
                scorekeeper->setO2LifebarRating(info->difficulty);
            } // else by default
            // LifebarType = LT_O2JAM; // By default, HX
            break;

        case LT_GROOVE:
        case LT_DEATH:
        case LT_EASY:
        case LT_EXHARD:
        case LT_SURVIVAL:
            if (chart_type == TI_BMS) {
                // Only needs setup if it's a BMS file
                if (const auto info = dynamic_cast<otoworm::BMSChartInfo *>(timing_info.get()); info->is_bmson)
                    scorekeeper->setLifeTotal(NAN, info->gauge_total / 100.0);
                else
                    scorekeeper->setLifeTotal(info->gauge_total);
            } else // by raindrop defaults
                scorekeeper->setLifeTotal(-1);
            // LifebarType = (LifeType)Parameters.GaugeType;
            break;
        case LT_NORECOV:
            // ...
            break;
        default:
            throw std::runtime_error("Invalid gauge type recieved");
    }
}

std::unique_ptr<Mechanics> configure_mechanics(
    PlayscreenParameters &param,
    const std::shared_ptr<otoworm::Chart> &current_chart,
    const std::shared_ptr<ScoreKeeper> &scorekeeper,
    const double judge_y) {
    std::unique_ptr<Mechanics> mechanics_set = nullptr;

    // This must be done before setLifeTotal in order for it to work.
    const auto transient = GetOtoTransient(current_chart);
    const auto object_count = transient ? transient->get_total_note_count() : 0;
    const auto score_count = transient ? transient->get_scorable_note_count() : 0;
    const auto hold_count = score_count - object_count;
    scorekeeper->setTotalObjects(object_count, hold_count);
    scorekeeper->setUseW0(param.UseW0);

    // JudgeScale, Stepmania and OD can't be run together - only one can be set.
    const auto timing_info = GetOtoTimingInfo(current_chart);

    // Pick a timing system
    if (param.SystemType == TI_NONE) {
        if (timing_info) {
            // Automatic setup
            param.SystemType = GetChartType(timing_info);
        } else {
            // Log::Printf("Null timing info - assigning raindrop defaults.\n");
            param.SystemType = TI_RAINDROP;
            // pick raindrop system for null Timing Info
        }
    }


    // If we got just assigned one or was already requested
    // unlikely: timing info type is none? what

    if (param.SystemType == TI_NONE) {
        // Player didn't request a specific subsystem
        // Log::Printf("System picked was none - on purpose. Defaulting to raindrop.\n");
        param.SystemType = TI_RAINDROP;
    }

    const TimingType used_timing_type = setup_game_system(param, timing_info, scorekeeper.get());

    /*
    If we're on TT_BEATS we've got to recalculate all note positions to beats,
    and use mechanics that use TT_BEATS as its timing type.
    */

    const bool disable_forced_release = param.SystemType == TI_BMS ||
                                        param.SystemType == TI_RDAC ||
                                        param.SystemType == TI_STEPMANIA;
    if (used_timing_type == TT_TIME) {
        if (param.SystemType == TI_RDAC) {
            // Log::Printf("RAINDROP ARCADE STAAAAAAAAART!\n");
            mechanics_set = std::make_unique<RaindropArcadeMechanics>();
        } else {
            // Log::Printf("Using raindrop mechanics set!\n");
            // Only forced release if not a bms or a stepmania chart.
            mechanics_set = std::make_unique<RaindropMechanics>(!disable_forced_release);
        }
        // ReSharper disable once CppDFAConstantConditions
    } else if (used_timing_type == TT_BEATS) {
        //Log::Printf("Using o2jam mechanics set!\n");
        mechanics_set = std::make_unique<O2JamMechanics>();
    }

    mechanics_set->configure(current_chart.get(), scorekeeper);
    setup_gauge(param, timing_info, scorekeeper.get());
    param.UpdateHidden(judge_y);

    return mechanics_set;
}


void PlayerContext::draw_barlines(const double current_vertical, const double user_speed_multiplier) const {
    for (const auto i: chart_state_.barlines) {
        const double real_v = (current_vertical - i * units_per_measure_) * user_speed_multiplier +
                              noteskin_->GetBarlineOffset() * sign(user_speed_multiplier) + get_judgment_y();
        if (real_v > 0 && real_v < ScreenWidth) {
            barline_->set_location(Vec2(noteskin_->GetBarlineStartX(), real_v),
                                  Vec2(noteskin_->GetBarlineStartX() + noteskin_->GetBarlineWidth(), real_v));
            barline_->render();
        }
    }
}

void PlayerContext::setup_mechanics() {
    mechanics_set_ = configure_mechanics(parameters_, current_chart_, player_score_keeper_, get_judgment_y());
    mechanics_set_->transform_notes(chart_state_);


    mechanics_set_->configure(current_chart_.get(), player_score_keeper_);
    // Setup mechanics set callbacks
    mechanics_set_->notify_hit = [this](auto &&PH1, auto &&PH2, auto &&PH3, auto &&PH4) {
        hit_note(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2), std::forward<decltype(PH3)>(PH3),
                 std::forward<decltype(PH4)>(PH4));
    };

    mechanics_set_->notify_miss = [this](auto &&PH1, auto &&PH2, auto &&PH3, auto &&PH4, auto &&PH5) {
        miss_note(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2), std::forward<decltype(PH3)>(PH3),
                  std::forward<decltype(PH4)>(PH4), std::forward<decltype(PH5)>(PH5));
    };

    mechanics_set_->is_lane_key_down = [this](auto &&PH1) {
        return get_gear_lane_state(std::forward<decltype(PH1)>(PH1));
    };

    mechanics_set_->set_lane_holding_state = [this](auto &&PH1, auto &&PH2) {
        set_lane_hold_state(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2));
    };

    mechanics_set_->play_keysound = play_keysound;
    // We're set - setup all the variables that depend on mechanics, scoring, etc.. to their initial values.
}


void PlayerContext::run_measures(const double time) {
    /*
        Notes are always run at unwarped time. GameChartData unwarps the time.
    */
    std::array<double, MAX_CHANNELS> time_closest{};
    auto perfect_auto = true;

    for (double &i: time_closest)
        i = std::numeric_limits<double>::infinity();

    const double used_time = get_chart_time_at(time);
    auto &notes_by_channel = chart_state_.notes_time_ordered;

    for (auto k = 0U; k < current_chart_->channels; k++) {
        for (auto mp = notes_by_channel[k].begin(); mp != notes_by_channel[k].end(); ++mp) {
            const auto m = chart_state_.note_at(k, *mp);
            if (!m) continue;

            // keysound update to closest note.
            if (m->is_enabled()) {
                if (const auto t = abs(used_time - m->get_end_time()); t < time_closest[k]) {
                    if (current_chart_->has_no_audio_stream)
                        gear_state_.current_keysounds[k] = m;
                    gear_state_.closest_note_timedist_ms[k] = abs(used_time - m->get_end_time());
                    time_closest[k] = t;
                }
            }

            if (!m->is_judgable() || !can_judge())
                continue;

            // Autoplay
            if (parameters_.Auto) {
                run_autoplay(m, used_time, k);
                if (!m->is_judgable()) continue;
            }

            if (!can_judge())
                continue; // don't check for judgments after stage has failed.

            if (mechanics_set_->OnUpdate(used_time, m, k))
                break;
        } // end for notes
    } // end for channels
}


void PlayerContext::play_lane_keysound(const uint32_t Lane) const {
    const auto TN = gear_state_.current_keysounds[Lane];
    if (!TN) return;

    play_keysound(TN->get_sound());
}

void PlayerContext::run_autoplay(RuntimeNote *m, const double usedTime, const uint32_t k) {
    const auto perfect_auto = true;
    if (const double time_threshold = usedTime + 0.008; m->get_start_time() <= time_threshold) {
        if (m->is_enabled()) {
            if (m->is_hold()) {
                if (m->was_hit()) {
                    if (m->get_end_time() < time_threshold) {
                        const double hit_time = clamp_to_interval(usedTime, m->get_end_time(), 0.008);
                        // We use clamp_to_interval for those pesky outliers.
                        if (perfect_auto) release_lane(k, m->get_end_time());
                        else release_lane(k, hit_time);
                    }
                } else {
                    const double hit_time = clamp_to_interval(usedTime, m->get_start_time(), 0.008);
                    if (perfect_auto) judge_lane(k, m->get_start_time());
                    else judge_lane(k, hit_time);
                }
            } else {
                const double hit_time = clamp_to_interval(usedTime, m->get_start_time(), 0.008);
                if (perfect_auto) {
                    judge_lane(k, m->get_start_time());
                    release_lane(k, m->get_end_time());
                } else {
                    judge_lane(k, hit_time);
                    release_lane(k, hit_time);
                }
            }
        }
    }
}


void PlayerContext::on_player_key_event(const double time, const bool key_down, const uint32_t lane) {
    replay_data_->add_event(Replay::Entry
        {
            time - drift_,
            lane,
            key_down
        });

    if (key_down) {
        judge_lane(lane, get_chart_time_at(time - drift_));
        gear_state_.is_key_down[lane] = true;
    } else {
        release_lane(lane, get_chart_time_at(time - drift_));
        gear_state_.is_key_down[lane] = false;
    }
}

void PlayerContext::update(const double song_time) {
    const auto drifted_time = song_time - drift_;
    const auto beat = chart_state_.get_beat_at(chart_state_.real_to_warped_time(drifted_time));
    noteskin_->update(song_time - last_update_time_, beat);
    last_update_time_ = song_time;
    replay_data_->update(song_time - drift_);
    run_measures(song_time - drift_);
}


void PlayerContext::render(const double song_time) {
    int rnc = draw_measures(song_time - drift_);
}

void PlayerContext::set_playable_data(std::shared_ptr<otoworm::Chart> chart, const double drift) {
    const CfgVar judge_offset_ms("JudgeOffsetMS");
    double desired_default_speed = Configuration::GetSkinConfigf("DefaultSpeedUnits");
    auto type = (ESpeedType) (int) Configuration::GetSkinConfigf("DefaultSpeedKind");

    current_chart_ = std::move(chart);
    this->drift_ = drift;
    judge_offset_ = judge_offset_ms / 1000.0;

    // this has to happen after the setup so we can use the effective parameters!
    // use data from the replay if one is loaded
    if (replay_data_->is_loaded()) {
        parameters_ = replay_data_->get_effective_parameters();
        desired_default_speed = parameters_.UserSpeedMultiplier;

        // treat desired default speed as the actual multiplier
        type = SPEEDTYPE_MULTIPLIER;

        replay_data_->add_playback_listener([this](const Replay::Entry entry) {
            this->on_player_key_event(entry.Time + this->get_drift(), entry.Down, entry.Lane);
        });
    }

    const auto d = setup(parameters_, desired_default_speed, type, drift, current_chart_);
    chart_state_ = *d;
    chart_state_.prepare_ordered_notes(); // Invalid ordered notes until we do this.
    delete d;

    setup_mechanics();

    // setupmechanics sets the effective gauge/etc so we have to do that first
    const auto transient = GetOtoTransient(current_chart_);
    replay_data_->set_chart_data(
        parameters_,
        type,
        transient ? transient->file_hash : "",
        transient ? transient->index_in_file : -1
    );

    units_per_measure_ = UNITS_PER_MEASURE;
}

bool PlayerContext::is_fail_enabled() const {
    return !parameters_.NoFail;
}

bool PlayerContext::is_upscrolling() const {
    return get_applied_speed_multiplier(last_update_time_ - drift_) < 0;
}

bool PlayerContext::get_uses_turntable() const {
    return chart_state_.has_turntable;
}

double PlayerContext::get_applied_speed_multiplier(const double time) const {
    const auto sm = chart_state_.get_speed_multiplier_at(time);
    if (parameters_.Upscroll)
        return -sm;
    else
        return sm;
}

double PlayerContext::get_current_beat() const {
    return chart_state_.get_beat_at(last_update_time_ - drift_);
}

double PlayerContext::get_user_multiplier() const {
    return parameters_.UserSpeedMultiplier;
}

double PlayerContext::get_current_vertical_speed() const {
    return chart_state_.get_displacement_speed_at(last_update_time_ - drift_);
}

double PlayerContext::get_warped_song_time() const {
    return chart_state_.real_to_warped_time(last_update_time_ - drift_);
}

double PlayerContext::get_current_bpm() const {
    return chart_state_.get_bpm_at(get_warped_song_time());
}

double PlayerContext::get_judgment_y() const {
    if (is_upscrolling())
        return noteskin_->GetJudgmentY();
    else
        return ScreenHeight - noteskin_->GetJudgmentY();
}

double PlayerContext::get_life_pst() const {
    const auto lifebar_type = get_current_gauge_type();
    const auto lifebar_amount = player_score_keeper_->getLifebarAmount(lifebar_type);
    if (lifebar_type == LT_GROOVE || lifebar_type == LT_EASY)
        return std::max(2, static_cast<int>(floor(lifebar_amount * 50) * 2));
    else
        return ceil(lifebar_amount * 50) * 2;
}

std::string PlayerContext::get_pacemaker_text(const bool bm) const {
    if (bm) {
        auto bmpm = player_score_keeper_->getAutoPacemaker();
        return bmpm.first;
    } else {
        auto pm = player_score_keeper_->getAutoRankPacemaker();
        return pm.first;
    }
}

int PlayerContext::get_pacemaker_value(const bool bm) const {
    if (bm) {
        const auto bmpm = player_score_keeper_->getAutoPacemaker();
        return bmpm.second;
    } else {
        const auto pm = player_score_keeper_->getAutoRankPacemaker();
        return pm.second;
    }
}

double PlayerContext::get_chart_time_at(const double time) const {
    if (mechanics_set_->GetTimingKind() == TT_BEATS) {
        return chart_state_.get_beat_at(time);
    } else
        return time;
}

int PlayerContext::get_channel_count() const {
    return chart_state_.chart->channels;
}

int PlayerContext::get_player_number() const {
    return player_number_;
}

bool PlayerContext::get_is_held_key(const int lane) const {
    if (lane >= 0 && lane < get_channel_count())
        return gear_state_.is_hold_active[lane];
    else
        return false;
}

bool PlayerContext::has_song_finished(const double time) const {
    const double wt = chart_state_.real_to_warped_time(time);
    double cutoff;

    if (player_score_keeper_->is_o2jam()) {
        // beat-based judgements
        const double cur_bps = chart_state_.get_bps_at(time);
        const double cutoffspb = 1 / cur_bps;

        cutoff = cutoffspb * player_score_keeper_->getLateMissCutoffMS();
    } else // time-based judgments
        cutoff = player_score_keeper_->getLateMissCutoffMS() / 1000.0;

    return wt > current_chart_->duration + cutoff;
}

double PlayerContext::get_waiting_time() const {
    const CfgVar waiting_time("WaitingTime");
    return std::max(std::max(waiting_time > 1.0 ? waiting_time : 1.5, 0.0),
                    current_chart_ ? -current_chart_->offset : 0);
}

otoworm::Chart *PlayerContext::get_chart() const {
    return current_chart_.get();
}

double PlayerContext::get_duration() const {
    return chart_state_.chart->duration;
}

double PlayerContext::get_beat_duration() const {
    return chart_state_.get_beat_at(get_duration());
}

ScoreKeeper *PlayerContext::get_score_keeper() const {
    return player_score_keeper_.get();
}

std::shared_ptr<ScoreKeeper> PlayerContext::get_score_keeper_shared() const {
    return player_score_keeper_;
}

double PlayerContext::get_closest_note_time(const int lane) const {
    if (lane >= 0 && lane < get_channel_count())
        return gear_state_.closest_note_timedist_ms[lane];
    else
        return std::numeric_limits<double>::infinity();
}

void PlayerContext::set_user_multiplier(const float multip) {
    parameters_.UserSpeedMultiplier = multip;
}

std::vector<otoworm::AutoplaySound> PlayerContext::create_autoplay_sound_list() {
    const CfgVar disable_keysounds("DisableKeysounds");

    // Load up BGM events
    std::vector<otoworm::AutoplaySound> autoplay_sounds;
    if (const auto transient = GetOtoTransient(current_chart_)) {
        autoplay_sounds.reserve(transient->bgm_events.size());
        for (const auto &bgm: transient->bgm_events)
            autoplay_sounds.push_back(bgm);
    }

    if (disable_keysounds)
        NoteTransform::MoveKeysoundsToBGM(current_chart_->channels, chart_state_.notes, autoplay_sounds, drift_);

    return autoplay_sounds;
}

void PlayerContext::setup_script_context(LuaManager *scripts) {
    assert(scripts != nullptr);

    luabridge::getGlobalNamespace(scripts->get_lua_state())
            /// @engineclass Player
            .beginClass<PlayerContext>("PlayerContext")
            /// Current song beat
            // @roproperty Beat
            .addProperty("Beat", &PlayerContext::get_current_beat)
            /// Song time in warped space
            // @roproperty Time
            .addProperty("Time", &PlayerContext::get_warped_song_time)
            /// Song duration in warp space
            // @roproperty Duration
            .addProperty("Duration", &PlayerContext::get_duration)
            /// Beat duration in warp space
            // @roproperty BeatDuration
            .addProperty("BeatDuration", &PlayerContext::get_beat_duration)
            /// Active channel count
            // @roproperty Channels
            .addProperty("Channels", &PlayerContext::get_channel_count)
            /// Current chart BPM
            // @roproperty BPM
            .addProperty("BPM", &PlayerContext::get_current_bpm)
            /// Whether failure is enabled
            // @roproperty CanFail
            .addProperty("CanFail", &PlayerContext::is_fail_enabled)
            /// Whether the player has failed
            // @roproperty HasFailed
            .addProperty("HasFailed", &PlayerContext::has_failed)
            /// Whether the notes are currently moving towards the top of the screen
            // @roproperty Upscroll
            .addProperty("Upscroll", &PlayerContext::is_upscrolling)
            /// The current displacement speed for the notes
            // @roproperty Speed
            .addProperty("Speed", &PlayerContext::get_current_vertical_speed)
            /// The current effective position of the judgment line
            // @roproperty JudgmentY
            .addProperty("JudgmentY", &PlayerContext::get_judgment_y)
            /// Whether the current chart is using a turntable
            // @roproperty Turntable
            .addProperty("Turntable", &PlayerContext::get_uses_turntable)
            /// Current speed multiplier
            // @property UserSpeedMultiplier
            .addProperty("UserSpeedMultiplier", &PlayerContext::get_user_multiplier,
                         &PlayerContext::set_user_multiplier)
            /// Same as Turntable
            // @roproperty HasTurntable
            .addProperty("HasTurntable", &PlayerContext::get_uses_turntable)
            /// Get current gauge health as a percentage
            // @roproperty LifebarPercent
            .addProperty("LifebarPercent", &PlayerContext::get_life_pst)
            /// Current player number
            // @roproperty Number
            .addProperty("Number", &PlayerContext::get_player_number)
            /// Get currently active score type's score value
            // @roproperty Score
            .addProperty("Score", &PlayerContext::get_score)
            /// Current player combo
            // @roproperty Combo
            .addProperty("Combo", &PlayerContext::get_combo)
            /// Get Pacemaker text (a la rank:+/-xxx - the rank part.)
            // @function GetPacemakerText
            // @param bm Whether to use the BMS grading pacemaker. Uses raindrop pacemaker otherwise
            .addFunction("GetPacemakerText", &PlayerContext::get_pacemaker_text)
            /// Get pacemaker value (a la rank:+/-xxx - the xxx part.)
            // @function GetPacemakerText
            // @param bm Whether to use the BMS grading pacemaker. Uses raindrop pacemaker otherwise
            .addFunction("GetPacemakerValue", &PlayerContext::get_pacemaker_value)
            /// Get if there's a hold currently being held at the given lane
            // @function IsHoldActive
            // @param lane Lane, 0-index based
            // @return A boolean, stating whether the lane has a hold currently being pressed.
            .addFunction("IsHoldActive", &PlayerContext::get_is_held_key)
            /// Get the closest note time to the last key press' timestamp.
            // @function GetClosestNoteTime
            // @param lane Lane, 0-index based
            // @return Closest note time, in MS.
            .addFunction("GetClosestNoteTime", &PlayerContext::get_closest_note_time)
            /// Get current player @{ScoreKeeper7K} instance
            // @roproperty Scorekeeper
            .addProperty("Scorekeeper", &PlayerContext::get_score_keeper)
            .endClass();
}

Replay PlayerContext::get_replay() const {
    return *replay_data_;
}

void PlayerContext::load_replay(const std::filesystem::path &path) const {
    replay_data_->load(path);
}

double PlayerContext::get_score() const {
    return player_score_keeper_->getScore(parameters_.GetScoringType());
}

int PlayerContext::get_combo() const {
    return player_score_keeper_->getScore(ST_COMBO);
}

void PlayerContext::hit_note(const double time_off,
                             const uint32_t lane,
                             const bool is_hold,
                             const bool is_hold_release) const {
    NoteJudgmentPart part;

    if (!is_hold) part = NoteJudgmentPart::NOTE;
    else {
        if (is_hold_release)
            part = NoteJudgmentPart::HOLD_TAIL;
        else
            part = NoteJudgmentPart::HOLD_HEAD;
    }

    const auto Judgment = player_score_keeper_->hitNote(time_off, lane, part);

    if (on_hit)
        on_hit(Judgment, time_off, lane, is_hold, is_hold_release, player_number_);
}

void PlayerContext::miss_note(
    const double time_off,
    const uint32_t lane,
    const bool is_hold,
    const bool dont_break_combo,
    const bool early_miss) {
    player_score_keeper_->missNote(dont_break_combo, early_miss, true);

    if (is_hold)
        gear_state_.is_hold_active[lane] = false;

    if (on_miss)
        on_miss(time_off, lane, is_hold, dont_break_combo, early_miss, player_number_);
}

void PlayerContext::gear_key_event(const uint32_t lane, const bool key_down) const {
    if (on_gear_key_event) {
        on_gear_key_event(lane, key_down, player_number_);
    }
}

void PlayerContext::judge_lane(const uint32_t lane, const double Time) {
    gear_key_event(lane, true);

    if (!can_judge())
        return;

    auto &notes = chart_state_.notes_time_ordered[lane];

    auto start = notes.begin();
    auto end = notes.end();

    // Use this optimization when we can make sure vertical properly aligns up with time, as with ReleaseLane.
    const auto threshold = (player_score_keeper_->is_o2jam()
                                ? player_score_keeper_->getJudgmentCutoffMS()
                                : (player_score_keeper_->getJudgmentCutoffMS() / 1000.0));
    const auto time_lower = (Time - threshold);
    const auto time_higher = (Time + threshold);

    auto note_start_before = [&](const RuntimeNoteHandle handle, const double time) {
        return chart_state_.note_at(lane, handle)->get_start_time() < time;
    };
    auto time_before_note_start = [&](const double time, const RuntimeNoteHandle handle) {
        return time < chart_state_.note_at(lane, handle)->get_start_time();
    };

    start = std::ranges::lower_bound(notes, time_lower, note_start_before);
    end = std::ranges::upper_bound(notes, time_higher, time_before_note_start);

    gear_state_.closest_note_timedist_ms[lane] = ms_display_margin_;

    for (auto mp = start; mp != end; ++mp) {
        const auto m = chart_state_.note_at(lane, *mp);
        if (!m) continue;
        const double dt = (Time - m->get_start_time()) * 1000;

        gear_state_.closest_note_timedist_ms[lane] = std::min(abs(dt), (double) gear_state_.closest_note_timedist_ms[lane]);

        if (gear_state_.closest_note_timedist_ms[lane] >= ms_display_margin_)
            gear_state_.closest_note_timedist_ms[lane] = 0;

        if (!m->is_judgable())
            continue;

        if (mechanics_set_->OnPressLane(Time, m, lane)) {
            return; // we judged a note in this lane, so we're done.
        }
    }

    if (gear_state_.current_keysounds[lane])
        play_keysound(gear_state_.current_keysounds[lane]->get_sound());
}

void PlayerContext::release_lane(const uint32_t Lane, const double Time) {
    gear_key_event(Lane, false);

    if (!can_judge()) return; // don't judge any more after stage is failed.

    auto &notes_by_channel = chart_state_.notes_time_ordered;
    auto start = notes_by_channel[Lane].begin();
    auto end = notes_by_channel[Lane].end();

    // Use this optimization when we can make sure vertical properly aligns up with time.
    //if (ChartState.IsNoteTimeSorted())
    // In comparison to the regular compare function, since end times are what matter with holds (or lift events, where start == end)
    // this does the job as it should instead of comparing start times where hold tails would be completely ignored.
    const auto threshold = (player_score_keeper_->is_o2jam()
                                ? player_score_keeper_->getLateMissCutoffMS()
                                : (player_score_keeper_->getLateMissCutoffMS() / 1000.0));

    const auto time_lower = (Time - threshold);
    const auto time_higher = (Time + threshold);

    auto note_end_before = [&](const RuntimeNoteHandle handle, const double time) {
        return chart_state_.note_at(Lane, handle)->get_end_time() < time;
    };
    auto time_before_note_end = [&](const double time, const RuntimeNoteHandle handle) {
        return time < chart_state_.note_at(Lane, handle)->get_end_time();
    };

    start = std::ranges::lower_bound(notes_by_channel[Lane]
                                     , time_lower, note_end_before);

    // Locate the first hold that we can judge in this range (Pending holds. Similar to what was done when drawing.)
    const auto rStart = std::reverse_iterator<RuntimeNoteHandleList::iterator>(start);
    for (auto i = rStart; i != notes_by_channel[Lane].rend(); ++i) {
        const auto ip = chart_state_.note_at(Lane, *i);
        if (!ip) continue;
        if (ip->is_hold()
            && ip->is_enabled()
            && ip->is_judgable()
            && ip->was_hit()
            && !ip->failed_hit())
            start = i.base() - 1;
    }

    end = std::ranges::upper_bound(notes_by_channel[Lane],
                                   time_higher,
                                   time_before_note_end);

    if (end != notes_by_channel[Lane].end())
        ++end;

    for (auto mp = start; mp != end; ++mp) {
        const auto m = chart_state_.note_at(Lane, *mp);
        if (!m) continue;
        if (!m->is_judgable()) continue;
        if (mechanics_set_->OnReleaseLane(Time, m, Lane)) // Are we done judging..?
            break;
    }
}

void PlayerContext::handle_lane_events(const int32_t key, const bool key_down, const double time) {
    if (parameters_.Auto)
        return;

    if (key < 0)
        return;

    if (!gear_state_.key_to_lane_bindings.contains(key))
        return;

    const int lane = gear_state_.key_to_lane_bindings[key]; /* Binding this key to a lane */

    if (lane >= MAX_CHANNELS || lane < 0)
        return;

    on_player_key_event(time + judge_offset_, key_down, lane);
}

void PlayerContext::set_lane_hold_state(const uint32_t Lane, const bool NewState) {
    gear_state_.is_hold_active[Lane] = NewState;
}

// true if holding down key
bool PlayerContext::get_gear_lane_state(const uint32_t Lane) const {
    return gear_state_.is_key_down[Lane] != 0;
}

bool PlayerContext::bind_keys_to_lanes(const bool use_turntable) {
    std::string key_profile;

    if (use_turntable)
        key_profile = (std::string) CfgVar("KeyProfileSpecial" + std::to_string(current_chart_->channels));
    else
        key_profile = (std::string) CfgVar("KeyProfile" + std::to_string(current_chart_->channels));

    const auto key_list = (std::string) CfgVar("Keys", key_profile);
    const auto key_list_arr = otoworm::util::token_split(key_list);

    for (unsigned i = 0; i < current_chart_->channels; i++) {
        gear_state_.closest_note_timedist_ms[i] = 0;

        if (i < key_list_arr.size())
            gear_state_.key_to_lane_bindings[static_cast<int>(latof(key_list_arr[i]))] = i;
        else {
            if (!parameters_.Auto) {
                Log::Printf("Mising bindings starting from lane " + std::to_string(i) + " using profile " +
                            key_profile);
                return false;
            }
        }

        gear_state_.is_hold_active[i] = false;
        gear_state_.is_key_down[i] = false;
    }

    return true;
}

void PlayerContext::set_can_judge(const bool can_judge) {
    judge_notes_ = can_judge;
}

bool PlayerContext::can_judge() const {
    return judge_notes_;
}

void PlayerContext::set_unwarped_time(const double time) {
    chart_state_.reset_notes();
    chart_state_.disable_notes_until(time);
}

int PlayerContext::get_current_gauge_type() const {
    return parameters_.GaugeType;
}


int PlayerContext::get_current_score_type() const {
    return parameters_.GetScoringType();
}

int PlayerContext::get_current_system_type() const {
    return mechanics_set_->GetTimingKind();
}

double PlayerContext::get_drift() const {
    return drift_;
}

double PlayerContext::get_judge_offset() const {
    return judge_offset_;
}

double PlayerContext::get_rate() const {
    return parameters_.Rate;
}

bool PlayerContext::has_failed() const {
    return player_score_keeper_->isStageFailed(get_current_gauge_type()) && !parameters_.NoFail;
}

bool PlayerContext::has_delayed_failure() const {
    return player_score_keeper_->hasDelayedFailure(get_current_gauge_type());
}

Mat4 id;

int PlayerContext::draw_measures(const double song_time) {
    int rnc = 0;
    /*
        DrawMeasures should get the unwarped song time.
        Internally, it uses warped song time.
    */
    const auto warped_time = chart_state_.real_to_warped_time(song_time);

    // note Y displacement at song_time
    const auto chart_displacement = chart_state_.get_displacement_at(warped_time) * units_per_measure_;

    // effective speed multiplier
    const auto chart_multiplier = get_applied_speed_multiplier(song_time);
    const auto effective_chart_speed_multiplier = chart_multiplier * parameters_.UserSpeedMultiplier;

    // since + is downward, - is upward!
    const bool upscrolling = effective_chart_speed_multiplier < 0;

    if (noteskin_->IsBarlineEnabled())
        draw_barlines(chart_displacement, effective_chart_speed_multiplier);

    // Set some parameters...
    renderer::set_default_shader_parameters(
        false,
        true,
        false,
        false,
        parameters_.GetHiddenMode()
    );

    // Sudden = 1, Hidden = 2, flashlight = 3 (Defined in the shader)
    if (parameters_.GetHiddenMode()) {
        renderer::Shader::set_uniform(
            renderer::DefaultShader::get_uniform(renderer::U_HIDCENTER),
            parameters_.GetHiddenCenter());
        renderer::Shader::set_uniform(
            renderer::DefaultShader::get_uniform(renderer::U_HIDSIZE),
            parameters_.GetHiddenTransitionSize());
        renderer::Shader::set_uniform(
            renderer::DefaultShader::get_uniform(renderer::U_HIDFLSIZE),
            parameters_.GetHiddenCenterSize());
    }

    renderer::set_primitive_quad_vbo();
    auto &notes = chart_state_.notes_vertically_ordered;
    const auto jy = get_judgment_y();

    for (auto k = 0U; k < current_chart_->channels; k++) {
        // From the note's vertical StaticVert transform to position on screen.
        auto calc_position = [&](const double static_vert) -> double {
            return (chart_displacement - static_vert * units_per_measure_) * effective_chart_speed_multiplier + jy;
        };

        auto start = notes[k].begin();
        auto end = notes[k].end();

        /* Find the location of the first/next visible regular note */
        auto loc_predicate = [&](const RuntimeNoteHandle handle, const double track_displacement) -> bool {
            const auto *a = chart_state_.note_at(k, handle);
            if (!upscrolling)
                return track_displacement < calc_position(a->get_vertical());
            else // Signs are switched. We need to preserve the same order.
                return track_displacement > calc_position(a->get_vertical());
        };

        // Signs are switched. Doesn't begin by the first note closest to the lower edge, but the one closest to the higher edge.
        if (!upscrolling)
            start = std::ranges::lower_bound(notes[k],
                                             ScreenHeight + noteskin_->GetNoteOffset(),
                                             loc_predicate);
        else
            start = std::ranges::lower_bound(notes[k],
                                             0 - noteskin_->GetNoteOffset(),
                                             loc_predicate);

        // Locate the first hold that we can draw in this range
        /*
            Since our object is on screen, our hold may be on screen but may not be this note
            since only head locations are used.
            Find this possible hold by checking if it intersects the screen.
        */
        if (start != notes[k].begin()) {
            auto i = start - 1; //std::reverse_iterator<std::vector<TrackNote>::iterator>(Start);
            const auto *note = chart_state_.note_at(k, *i);
            if (note && note->is_hold() && note->is_visible()) {
                auto vert = calc_position(note->get_vertical());
                auto vert_end = calc_position(note->get_hold_end_vertical());
                if (intervals_intersect(0, ScreenHeight, std::min(vert, vert_end), std::max(vert, vert_end))) {
                    start = i;
                }
            }
        }

        // Find the note that is out of the drawing range
        // As before. Top becomes bottom, bottom becomes top.
        if (!upscrolling)
            end = std::ranges::lower_bound(notes[k], 0 - noteskin_->GetNoteOffset(),
                                           loc_predicate);
        else
            end = std::ranges::lower_bound(notes[k],
                                           ScreenHeight + noteskin_->GetNoteOffset(), loc_predicate);


        // Now, draw them.
        for (auto mp = start; mp != end; ++mp) {
            const auto m = chart_state_.note_at(k, *mp);
            if (!m) continue;

            // Don't attempt drawing this object if not visible.
            if (!m->is_visible())
                continue;

            const double vertical = calc_position(m->get_vertical());
            const double vertical_hold_end = calc_position(m->get_hold_end_vertical());

            double judge_y;

            // LR2 style keep-on-the-judgment-line
            if (const bool above_line = vertical < get_judgment_y(); !(above_line ^ upscrolling) && m->is_judgable())
                judge_y = get_judgment_y();
            else
                judge_y = vertical;

            // We draw the body first, so that way the heads get drawn on top
            if (m->is_hold()) {
                // todo: move this note state determination to the note itself
                enum : int {
                    Failed, Active, BeingHit, SuccesfullyHit
                };
                int level = -1;

                if (m->is_enabled() && !m->failed_hit())
                    level = Active;
                if (!m->is_enabled() && m->failed_hit())
                    level = Failed;
                if (!m->is_enabled() && !m->failed_hit() && !m->was_hit())
                    level = Failed;
                if (m->is_enabled() && m->was_hit() && !m->failed_hit())
                    level = BeingHit;
                if (!m->is_enabled() && m->was_hit() && !m->failed_hit())
                    level = SuccesfullyHit;

                // If we're being hit and..
                const bool decrease_hold_size = noteskin_->ShouldDecreaseHoldSizeWhenBeingHit() && level == 2;
                auto reference_point = 0.0f;
                if (decrease_hold_size) {
                    reference_point = judge_y;
                } else // We were failed, not being hit or were already hit
                {
                    reference_point = vertical;
                }

                const double pos = (vertical_hold_end + reference_point) / 2;
                const double size = vertical_hold_end - reference_point;

                noteskin_->DrawHoldBody(k, pos, size, level);
                noteskin_->DrawHoldTail(*m, k, vertical_hold_end, level);

                if (noteskin_->AllowDanglingHeads() || decrease_hold_size)
                    noteskin_->DrawHoldHead(*m, k, judge_y, level);
                else
                    noteskin_->DrawHoldHead(*m, k, vertical, level);
            } else {
                if (noteskin_->AllowDanglingHeads())
                    noteskin_->DrawNote(*m, k, judge_y);
                else
                    noteskin_->DrawNote(*m, k, vertical);
            }

            rnc++; // Rendered note count increases...
        }
    }

    /* Clean up */
    renderer::set_default_shader_parameters(false, true, false, false, 0);
    renderer::finalize_draw();


    if (DebugNoteRendering) {
        fnt->render(otoworm::util::format(
                        "NOTES RENDERED: %d\nRNG: (%f to %f)\nMULT/EFFECTIVEMULT/SPEED: %f/%f/%f",
                        rnc,
                        chart_displacement, chart_displacement + ScreenHeight,
                        chart_multiplier, effective_chart_speed_multiplier,
                        get_current_vertical_speed() * effective_chart_speed_multiplier), Vec2(0, 0));
    }
    return rnc;
}
