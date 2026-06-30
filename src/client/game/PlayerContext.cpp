#include <rmath.h>
#include <queue>
#include <game/Song.h>
#include <game/ScoreKeeper7K.h>
#include <game/RaindropProcessedChart.h>
#include <game/VSRGMechanics.h>
#include <game/NoteTransformations.h>


#include <glm.h>
#include <TextAndFileUtil.h>

#include "LuaManager.h"
#include <LuaBridge/LuaBridge.h>

#include "PlayscreenParameters.h"
#include "PlayerContext.h"

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
#include "Replay7K.h"

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

    ChartType GetChartType(const std::shared_ptr<otoworm::ChartInfo>& timing_info) {
        return timing_info ? ToRdChartType(timing_info->get_class()) : TI_NONE;
    }

    std::shared_ptr<otoworm::ChartInfo> GetOtoTimingInfo(const std::shared_ptr<otoworm::Chart>& chart) {
        if (!chart || !chart->transient)
            return nullptr;

        return chart->transient->specialized_info;
    }

    otoworm::ChartTransient* GetOtoTransient(const std::shared_ptr<otoworm::Chart>& chart) {
        if (!chart)
            return nullptr;

        return chart->transient.get();
    }

    AutoplaySound ToRdAutoplaySound(const otoworm::AutoplaySound& sound) {
        return {sound.time, sound.sound};
    }
}

PlayerContext::PlayerContext(int pn, PlayscreenParameters p) : ChartState(DEFAULT_WAIT_TIME) {
    PlayerNoteskin = new Noteskin(this);
    PlayerReplay = new Replay();
    PlayerNumber = pn;
    Drift = 0;
    JudgeOffset = 0;

    JudgeNotes = true;

    PlayerScoreKeeper = std::make_shared<rd::ScoreKeeper>();
    Parameters = p;

    Gear = {};

    if (!fnt && DebugNoteRendering) {
        fnt = new BitmapFont();
        fnt->load_skin_font_image("font.tga", Vec2(6, 15), Vec2(8, 16), Vec2(6, 15), 0);
    }

    Barline = nullptr;
}

PlayerContext::~PlayerContext() {
    if (Barline) {
        delete Barline;
        Barline = nullptr;
    }

    delete PlayerNoteskin;
    delete PlayerReplay;
}

void PlayerContext::init() const
{
    PlayerNoteskin->SetupNoteskin(ChartState.has_turntable, CurrentChart->channels);
}

void PlayerContext::validate() {
    PlayerNoteskin->Validate();
    MsDisplayMargin = (Configuration::GetSkinConfigf("HitErrorDisplayLimiter"));


    if (!BindKeysToLanes(ChartState.has_turntable))
        if (!BindKeysToLanes(!ChartState.has_turntable))
            Log::LogPrintf("Couldn't get valid bindings for current key count %d.\n", GetChannelCount());

    if (PlayerNoteskin->IsBarlineEnabled())
        Barline = new Line();
}


const RaindropProcessedChart &PlayerContext::get_chart_state() {
    return ChartState;
}

TimingType setup_game_system(
        PlayscreenParameters &param,
        const std::shared_ptr<otoworm::ChartInfo>& timing_info,
        ScoreKeeper* PlayerScoreKeeper) {
    TimingType UsedTimingType = TT_TIME;
    const auto chart_type = GetChartType(timing_info);

    if (param.SystemType == TI_BMS || param.SystemType == TI_RDAC || param.SystemType == TI_LR2) {
        UsedTimingType = TT_TIME;
        if (chart_type == TI_BMS) {
            auto info = static_cast<otoworm::BMSChartInfo*> (timing_info.get());
            if (!info->percentual_judgerank)
                PlayerScoreKeeper->setJudgeRank(info->judge_rank);
            else
                PlayerScoreKeeper->setJudgeScale(info->judge_rank / 100.0);
        }
        else {
            PlayerScoreKeeper->setJudgeRank(2);
        }

        if (param.SystemType == TI_LR2) {
            PlayerScoreKeeper->useLR2Timing();
        }
    }
    else if (param.SystemType == TI_O2JAM) {
        UsedTimingType = TT_BEATS;
        PlayerScoreKeeper->setJudgeRank(-100);
    }
    else if (param.SystemType == TI_OSUMANIA) {
        UsedTimingType = TT_TIME;
        if (chart_type == TI_OSUMANIA) {
            auto info = static_cast<otoworm::OsumaniaChartInfo*> (timing_info.get());
            PlayerScoreKeeper->setODWindows(info->od);
        }
        else PlayerScoreKeeper->setODWindows(7);
    }
    else if (param.SystemType == TI_STEPMANIA) {
        UsedTimingType = TT_TIME;
        PlayerScoreKeeper->setSMJ4Windows();
    }
    else if (param.SystemType == TI_RAINDROP) {
        // LifebarType = LT_STEPMANIA;
    } else {
        Log::LogPrintf("Warning: unknown SystemType %d\n", param.SystemType);
    }

    PlayerScoreKeeper->applyRateScale(param.Rate);
    return UsedTimingType;
}


RaindropProcessedChart* Setup(
        PlayscreenParameters &param,
        double DesiredDefaultSpeed,
        int Type,
        double drift,
        const std::shared_ptr<otoworm::Chart>& current_chart
)
{
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

    if (DesiredDefaultSpeed != 0)
    {
        DesiredDefaultSpeed /= param.Rate;

        if (Type == SPEEDTYPE_CMOD) // cmod
        {
            param.UserSpeedMultiplier = 1;

            double spd = param.GreenNumber ? 1000 : DesiredDefaultSpeed;

            chart_state = RaindropProcessedChart::from(current_chart.get(), spd);
        }
        else
            chart_state = RaindropProcessedChart::from(current_chart.get());

        // if GN is true, Default Speed = GN!
        // Convert GN to speed.
        if (param.GreenNumber) {

            // v0 is normal speed
            // (green number speed * green number time) / (normal speed * normal time)
            // equals
            // playfield distance / note distance

            // simplifying that gets you dn / tn lol
            double new_speed = PLAYFIELD_SIZE / (DesiredDefaultSpeed / 1000);
            DesiredDefaultSpeed = new_speed;

            if (Type == SPEEDTYPE_CMOD) {
                param.UserSpeedMultiplier = DesiredDefaultSpeed / 1000;
            }
        }

        if (Type == SPEEDTYPE_MMOD) // mmod
        {
            double speed_max = 0; // Find the highest speed
            for (auto i : chart_state.speeds)
            {
                speed_max = std::max(speed_max, abs(i.value));
            }

            double Ratio = DesiredDefaultSpeed / speed_max; // How much above or below are we from the maximum speed?
            param.UserSpeedMultiplier = Ratio;
        }
        else if (Type == SPEEDTYPE_FIRST) // First speed.
        {
            double DesiredMultiplier = DesiredDefaultSpeed / chart_state.speeds[0].value;
            param.UserSpeedMultiplier = DesiredMultiplier;
        }
        else if (Type == SPEEDTYPE_MODE) // Most lasting speed.
        {
            std::map <double, double> freq;
            for (auto i = chart_state.speeds.begin(); i != chart_state.speeds.end(); i++)
            {
                if (i + 1 != chart_state.speeds.end())
                {
                    freq[i->value] += (i + 1)->time - i->time;
                }
                else freq[i->value] += abs(current_chart->duration - i->time);
            }
            auto max = -std::numeric_limits<float>::infinity();
            auto val = 1000.f;
            for (auto i : freq)
            {
                if (i.second > max)
                {
                    max = i.second;
                    val = i.first;
                }
            }

            param.UserSpeedMultiplier = DesiredDefaultSpeed  / (val * UNITS_PER_MEASURE);
        }
        else if (Type == SPEEDTYPE_MULTIPLIER) // target speed is just a multiplier
        {
            param.UserSpeedMultiplier = DesiredDefaultSpeed;
        }
        else if (Type != SPEEDTYPE_CMOD) // other cases
        {
            double bpsd = 4.0 / (chart_state.bps[0].value);
            double Speed = (UNITS_PER_MEASURE / bpsd);
            double DesiredMultiplier = DesiredDefaultSpeed / Speed;

            param.UserSpeedMultiplier = DesiredMultiplier;
        }
    }
    else
        chart_state = RaindropProcessedChart::from(current_chart.get(), drift);

    if (param.Random) {
        if (!param.IsSeedSet)
            param.SetSeed(time(nullptr));


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

void setup_gauge(PlayscreenParameters& param, const std::shared_ptr<otoworm::ChartInfo>& timing_info, ScoreKeeper* PlayerScoreKeeper)
{
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
                auto info = static_cast<otoworm::OsumaniaChartInfo*> (timing_info.get());
                PlayerScoreKeeper->setOsuHP(info->hp);
            }

        case LT_O2JAM:
            if (chart_type == TI_O2JAM) {
                auto info = static_cast<otoworm::O2JamChartInfo*> (timing_info.get());
                PlayerScoreKeeper->setO2LifebarRating(info->difficulty);
            } // else by default
            // LifebarType = LT_O2JAM; // By default, HX
            break;

        case LT_GROOVE:
        case LT_DEATH:
        case LT_EASY:
        case LT_EXHARD:
        case LT_SURVIVAL:
            if (chart_type == TI_BMS) { // Only needs setup if it's a BMS file
                auto info = static_cast<otoworm::BMSChartInfo*> (timing_info.get());
                if (info->is_bmson)
                    PlayerScoreKeeper->setLifeTotal(NAN, info->gauge_total / 100.0);
                else
                    PlayerScoreKeeper->setLifeTotal(info->gauge_total);
            }
            else // by raindrop defaults
                PlayerScoreKeeper->setLifeTotal(-1);
            // LifebarType = (LifeType)Parameters.GaugeType;
            break;
        case LT_NORECOV:
            // ...
            break;
        default:
            throw std::runtime_error("Invalid gauge type recieved");
    }

}

std::unique_ptr<rd::Mechanics> PrepareMechanicsSet(
        PlayscreenParameters& param,
        const std::shared_ptr<otoworm::Chart>& CurrentChart,
        const std::shared_ptr<rd::ScoreKeeper>& PlayerScoreKeeper,
        double JudgeY)
{
    std::unique_ptr<rd::Mechanics> MechanicsSet = nullptr;

    // This must be done before setLifeTotal in order for it to work.
    const auto transient = GetOtoTransient(CurrentChart);
    const auto object_count = transient ? transient->get_total_note_count() : 0;
    const auto score_count = transient ? transient->get_scorable_note_count() : 0;
    auto hold_count = score_count - object_count;
    PlayerScoreKeeper->setTotalObjects(object_count, hold_count);

    PlayerScoreKeeper->setUseW0(param.UseW0);

    // JudgeScale, Stepmania and OD can't be run together - only one can be set.
    auto timing_info = GetOtoTimingInfo(CurrentChart);

    // Pick a timing system
    if (param.SystemType == TI_NONE) {
        if (timing_info) {
            // Automatic setup
            param.SystemType = GetChartType(timing_info);
        }
        else {
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

    TimingType used_timing_type = setup_game_system(param, timing_info, PlayerScoreKeeper.get());

    /*
    If we're on TT_BEATS we've got to recalculate all note positions to beats,
    and use mechanics that use TT_BEATS as its timing type.
    */

    bool disable_forced_release = param.SystemType == TI_BMS ||
                                  param.SystemType == TI_RDAC ||
                                  param.SystemType == TI_STEPMANIA;
    if (used_timing_type == TT_TIME)
    {
        if (param.SystemType == TI_RDAC)
        {
            // Log::Printf("RAINDROP ARCADE STAAAAAAAAART!\n");
            MechanicsSet = std::make_unique<RaindropArcadeMechanics>();
        }
        else {
            // Log::Printf("Using raindrop mechanics set!\n");
            // Only forced release if not a bms or a stepmania chart.
            MechanicsSet = std::make_unique<RaindropMechanics>(!disable_forced_release);
        }
    }
    else if (used_timing_type == TT_BEATS)
    {
        //Log::Printf("Using o2jam mechanics set!\n");
        MechanicsSet = std::make_unique<O2JamMechanics>();
    }

    MechanicsSet->Setup(CurrentChart.get(), PlayerScoreKeeper);
    setup_gauge(param, timing_info, PlayerScoreKeeper.get());
    param.UpdateHidden(JudgeY);

    return MechanicsSet;
}


void PlayerContext::SetupMechanics() {
    MechanicsSet = PrepareMechanicsSet(Parameters, CurrentChart, PlayerScoreKeeper, GetJudgmentY());
    MechanicsSet->TransformNotes(ChartState);


    MechanicsSet->Setup(CurrentChart.get(), PlayerScoreKeeper);
    // Setup mechanics set callbacks
    MechanicsSet->HitNotify = [this](auto && PH1, auto && PH2, auto && PH3, auto && PH4) { hit_note(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2), std::forward<decltype(PH3)>(PH3), std::forward<decltype(PH4)>(PH4)); };

    MechanicsSet->MissNotify = [this](auto && PH1, auto && PH2, auto && PH3, auto && PH4, auto && PH5) { miss_note(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2), std::forward<decltype(PH3)>(PH3), std::forward<decltype(PH4)>(PH4), std::forward<decltype(PH5)>(PH5)); };

    MechanicsSet->IsLaneKeyDown = [this](auto && PH1) { return get_gear_lane_state(std::forward<decltype(PH1)>(PH1)); };

    MechanicsSet->SetLaneHoldingState = [this](auto && PH1, auto && PH2) { set_lane_hold_state(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2)); };

    MechanicsSet->PlayNoteSoundEvent = PlayKeysound;
    // We're set - setup all of the variables that depend on mechanics, scoring etc.. to their initial values.
}

void PlayerContext::on_player_key_event(double Time, bool KeyDown, uint32_t lane) {
    PlayerReplay->AddEvent(Replay::Entry
                                  {
                                          Time - Drift,
                                          lane,
                                          KeyDown
                                  });

    if (KeyDown) {
        judge_lane(lane, GetChartTimeAt(Time - Drift));
        Gear.IsPressed[lane] = true;
    } else {
        release_lane(lane, GetChartTimeAt(Time - Drift));
        Gear.IsPressed[lane] = false;
    }
}


void PlayerContext::translate_key(int32_t Index, bool KeyDown, double Time) {
    if (Parameters.Auto)
        return;

    if (Index < 0)
        return;

    if (!Gear.Bindings.contains(Index))
        return;

    int GearIndex = Gear.Bindings[Index]; /* Binding this key to a lane */

    if (GearIndex >= rd::MAX_CHANNELS || GearIndex < 0)
        return;

    on_player_key_event(Time + JudgeOffset, KeyDown, GearIndex);
}


bool PlayerContext::is_fail_enabled() const {
    return !Parameters.NoFail;
}

bool PlayerContext::is_upscrolling() const {
    return GetAppliedSpeedMultiplier(LastUpdateTime - Drift) < 0;
}


double PlayerContext::GetCurrentBPM() const {
    return ChartState.get_bpm_at(get_warped_song_time());
}

double PlayerContext::GetJudgmentY() const {
    if (is_upscrolling())
        return PlayerNoteskin->GetJudgmentY();
    else
        return ScreenHeight - PlayerNoteskin->GetJudgmentY();
}


otoworm::Chart *PlayerContext::get_chart() const {
    return CurrentChart.get();
}

double PlayerContext::get_duration() const {
    return ChartState.chart->duration;
}

double PlayerContext::get_beat_duration() const {
    return ChartState.get_beat_at(get_duration());
}

int PlayerContext::GetChannelCount() const {
    return ChartState.chart->channels;
}

int PlayerContext::get_player_number() const {
    return PlayerNumber;
}

bool PlayerContext::get_is_held_key(int lane) const {
    if (lane >= 0 && lane < GetChannelCount())
        return Gear.HeldKey[lane];
    else
        return false;
}

bool PlayerContext::get_uses_turntable() const {
    return ChartState.has_turntable;
}

double PlayerContext::GetAppliedSpeedMultiplier(double Time) const {
    auto sm = ChartState.get_speed_multiplier_at(Time);
    if (Parameters.Upscroll)
        return -sm;
    else
        return sm;
}

double PlayerContext::get_current_beat() const {
    return ChartState.get_beat_at(LastUpdateTime - Drift);
}

double PlayerContext::get_user_multiplier() const {
    return Parameters.UserSpeedMultiplier;
}

double PlayerContext::get_current_vertical_speed() const {
    return ChartState.get_displacement_speed_at(LastUpdateTime - Drift);
}

double PlayerContext::get_warped_song_time() const {
    return ChartState.real_to_warped_time(LastUpdateTime - Drift);
}

void PlayerContext::SetupLua(LuaManager *Env) {
    assert(Env != nullptr);

    luabridge::getGlobalNamespace(Env->GetState())
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
            .addProperty("Channels", &PlayerContext::GetChannelCount)
                    /// Current chart BPM
                    // @roproperty BPM
            .addProperty("BPM", &PlayerContext::GetCurrentBPM)
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
            .addProperty("JudgmentY", &PlayerContext::GetJudgmentY)
                    /// Whether the current chart is using a turntable
                    // @roproperty Turntable
            .addProperty("Turntable", &PlayerContext::get_uses_turntable)
                    /// Current speed multiplier
                    // @property UserSpeedMultiplier
            .addProperty("UserSpeedMultiplier", &PlayerContext::get_user_multiplier,
                         &PlayerContext::SetUserMultiplier)
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
            .addProperty("Score", &PlayerContext::GetScore)
                    /// Current player combo
                    // @roproperty Combo
            .addProperty("Combo", &PlayerContext::GetCombo)
                    /// Get Pacemaker text (a la rank:+/-xxx - the rank part.)
                    // @function GetPacemakerText
                    // @param bm Whether to use the BMS grading pacemaker. Uses raindrop pacemaker otherwise
            .addFunction("GetPacemakerText", &PlayerContext::GetPacemakerText)
                    /// Get pacemaker value (a la rank:+/-xxx - the xxx part.)
                    // @function GetPacemakerText
                    // @param bm Whether to use the BMS grading pacemaker. Uses raindrop pacemaker otherwise
            .addFunction("GetPacemakerValue", &PlayerContext::GetPacemakerValue)
                    /// Get if there's a hold currently being held at the given lane
                    // @function IsHoldActive
                    // @param lane Lane, 0-index based
                    // @return A boolean, stating whether the lane has a hold currently being pressed.
            .addFunction("IsHoldActive", &PlayerContext::get_is_held_key)
                    /// Get the closest note time to the last key press' timestamp.
                    // @function GetClosestNoteTime
                    // @param lane Lane, 0-index based
                    // @return Closest note time, in MS.
            .addFunction("GetClosestNoteTime", &PlayerContext::GetClosestNoteTime)
                    /// Get current player @{ScoreKeeper7K} instance
                    // @roproperty Scorekeeper
            .addProperty("Scorekeeper", &PlayerContext::GetScoreKeeper)
            .endClass();
}

Replay PlayerContext::get_replay() const
{
    return *PlayerReplay;
}

void PlayerContext::load_replay(std::filesystem::path path) const
{
    PlayerReplay->Load(path);
}

double PlayerContext::GetScore() const {
    return PlayerScoreKeeper->getScore(Parameters.GetScoringType());
}

int PlayerContext::GetCombo() const {
    return PlayerScoreKeeper->getScore(ST_COMBO);
}

bool PlayerContext::BindKeysToLanes(bool UseTurntable) {
    std::string KeyProfile;
    std::string keyList;
    std::vector<std::string> keyListArr;

    if (UseTurntable)
        KeyProfile = (std::string) CfgVar("KeyProfileSpecial" + IntToStr(CurrentChart->channels));
    else
        KeyProfile = (std::string) CfgVar("KeyProfile" + IntToStr(CurrentChart->channels));

    keyList = (std::string) CfgVar("Keys", KeyProfile);
    keyListArr = Utility::TokenSplit(keyList);

    for (unsigned i = 0; i < CurrentChart->channels; i++) {
        Gear.ClosestNoteMS[i] = 0;

        if (i < keyListArr.size())
            Gear.Bindings[static_cast<int>(latof(keyListArr[i]))] = i;
        else {
            if (!Parameters.Auto) {
                Log::Printf("Mising bindings starting from lane " + IntToStr(i) + " using profile " +
                            KeyProfile);
                return false;
            }
        }

        Gear.HeldKey[i] = false;
        Gear.IsPressed[i] = false;
    }

    return true;
}

void PlayerContext::hit_note(double TimeOff, uint32_t Lane, bool IsHold, bool IsHoldRelease) const
{

    NoteJudgmentPart part;

    if (!IsHold) part = NoteJudgmentPart::NOTE;
    else {
        if (IsHoldRelease)
            part = NoteJudgmentPart::HOLD_TAIL;
        else
            part = NoteJudgmentPart::HOLD_HEAD;
    }

    auto Judgment = PlayerScoreKeeper->hitNote(TimeOff, Lane, part);

    if (OnHit)
        OnHit(Judgment, TimeOff, Lane, IsHold, IsHoldRelease, PlayerNumber);
}

void PlayerContext::miss_note(double TimeOff, uint32_t Lane, bool IsHold, bool dont_break_combo, bool early_miss) {
    PlayerScoreKeeper->missNote(dont_break_combo, early_miss, true);

    if (IsHold)
        Gear.HeldKey[Lane] = false;

    if (OnMiss)
        OnMiss(TimeOff, Lane, IsHold, dont_break_combo, early_miss, PlayerNumber);
}

void PlayerContext::set_lane_hold_state(uint32_t Lane, bool NewState) {
    Gear.HeldKey[Lane] = NewState;
}

void PlayerContext::play_lane_keysound(uint32_t Lane) const
{
    auto TN = Gear.CurrentKeysounds[Lane];
    if (!TN) return;

    PlayKeysound(TN->get_sound());
}

double PlayerContext::GetChartTimeAt(double time) const {
    if (MechanicsSet->GetTimingKind() == TT_BEATS) {
        return ChartState.get_beat_at(time);
    }
        /*else if (MechanicsSet->GetTimingKind() == TT_TIME) {
            return time;
        }*/
    else
        return time;
}

// true if holding down key
bool PlayerContext::get_gear_lane_state(uint32_t Lane) const
{
    return Gear.IsPressed[Lane] != 0;
}

void PlayerContext::RunAuto(RuntimeNote *m, double usedTime, uint32_t k) {
    auto perfect_auto = true;
    if (double TimeThreshold = usedTime + 0.008; m->get_start_time() <= TimeThreshold) {
        if (m->is_enabled()) {
            if (m->is_hold()) {
                if (m->was_hit()) {
                    if (m->get_end_time() < TimeThreshold) {
                        double hit_time = clamp_to_interval(usedTime, m->get_end_time(), 0.008);
                        // We use clamp_to_interval for those pesky outliers.
                        if (perfect_auto) release_lane(k, m->get_end_time());
                        else release_lane(k, hit_time);
                    }
                } else {
                    double hit_time = clamp_to_interval(usedTime, m->get_start_time(), 0.008);
                    if (perfect_auto) judge_lane(k, m->get_start_time());
                    else judge_lane(k, hit_time);
                }
            } else {
                double hit_time = clamp_to_interval(usedTime, m->get_start_time(), 0.008);
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

void PlayerContext::RunMeasures(double time) {
    /*
        Notes are always ran at unwarped time. GameChartData unwarps the time.
    */
    double timeClosest[MAX_CHANNELS];
    auto perfect_auto = true;

    for (double & i : timeClosest)
        i = std::numeric_limits<double>::infinity();

    double usedTime = GetChartTimeAt(time);
    auto &NotesByChannel = ChartState.notes_time_ordered;

    for (auto k = 0U; k < CurrentChart->channels; k++) {
        for (auto mp = NotesByChannel[k].begin(); mp != NotesByChannel[k].end(); ++mp) {
            auto m = ChartState.note_at(k, *mp);
            if (!m) continue;
            // Keysound update to closest note.
            if (m->is_enabled()) {
                auto t = abs(usedTime - m->get_end_time());
                if (t < timeClosest[k]) {
                    if (CurrentChart->has_no_audio_stream)
                        Gear.CurrentKeysounds[k] = m;
                    Gear.ClosestNoteMS[k] = abs(usedTime - m->get_end_time());
                    timeClosest[k] = t;
                }
            }

            if (!m->is_judgable() || !CanJudge())
                continue;

            // Autoplay
            if (Parameters.Auto) {
                RunAuto(m, usedTime, k);
                if (!m->is_judgable()) continue;
            }

            if (!CanJudge()) continue; // don't check for judgments after stage has failed.

            if (MechanicsSet->OnUpdate(usedTime, m, k))
                break;
        } // end for notes
    } // end for channels
}

void PlayerContext::release_lane(uint32_t Lane, double Time) {
    GearKeyEvent(Lane, false);

    if (!CanJudge()) return; // don't judge any more after stage is failed.

    auto &NotesByChannel = ChartState.notes_time_ordered;
    auto Start = NotesByChannel[Lane].begin();
    auto End = NotesByChannel[Lane].end();

    // Use this optimization when we can make sure vertical properly aligns up with time.
    //if (ChartState.IsNoteTimeSorted())
    {
        // In comparison to the regular compare function, since end times are what matter with holds (or lift events, where start == end)
        // this does the job as it should instead of comparing start times where hold tails would be completely ignored.
        auto threshold = (PlayerScoreKeeper->usesO2() ?
                          PlayerScoreKeeper->getLateMissCutoffMS() :
                          (PlayerScoreKeeper->getLateMissCutoffMS() / 1000.0));

        auto timeLower = (Time - threshold);
        auto timeHigher = (Time + threshold);

        auto note_end_before = [&](const RuntimeNoteHandle handle, const double time) {
            return ChartState.note_at(Lane, handle)->get_end_time() < time;
        };
        auto time_before_note_end = [&](const double time, const RuntimeNoteHandle handle) {
            return time < ChartState.note_at(Lane, handle)->get_end_time();
        };

        Start = std::ranges::lower_bound(NotesByChannel[Lane]
                                         , timeLower, note_end_before);

        // Locate the first hold that we can judge in this range (Pending holds. Similar to what was done when drawing.)
        auto rStart = std::reverse_iterator<RuntimeNoteHandleList::iterator>(Start);
        for (auto i = rStart; i != NotesByChannel[Lane].rend(); ++i) {
            auto ip = ChartState.note_at(Lane, *i);
            if (!ip) continue;
            if (ip->is_hold()
                && ip->is_enabled()
                && ip->is_judgable()
                && ip->was_hit()
                && !ip->failed_hit())
                Start = i.base() - 1;
        }

        End = std::ranges::upper_bound(NotesByChannel[Lane]
                                       ,
                                       timeHigher,
                                       time_before_note_end);

        if (End != NotesByChannel[Lane].end())
            ++End;
    }

    for (auto mp = Start; mp != End; ++mp) {
        auto m = ChartState.note_at(Lane, *mp);
        if (!m) continue;
        if (!m->is_judgable()) continue;
        if (MechanicsSet->OnReleaseLane(Time, m, Lane)) // Are we done judging..?
            break;
    }
}

ScoreKeeper *PlayerContext::GetScoreKeeper() const {
    return PlayerScoreKeeper.get();
}

std::shared_ptr<ScoreKeeper> PlayerContext::GetScoreKeeperShared() const {
    return PlayerScoreKeeper;
}

void PlayerContext::SetCanJudge(bool canjudge) {
    JudgeNotes = canjudge;
}

bool PlayerContext::CanJudge() {
    return JudgeNotes;
}

void PlayerContext::SetUnwarpedTime(double time) {
    ChartState.reset_notes();
    ChartState.disable_notes_until(time);
}

int PlayerContext::GetCurrentGaugeType() const {
    return Parameters.GaugeType;
}

int PlayerContext::get_current_score_type() const {
    return Parameters.GetScoringType();
}

int PlayerContext::get_current_system_type() const {
    return MechanicsSet->GetTimingKind();
}

double PlayerContext::get_drift() const {
    return Drift;
}

double PlayerContext::get_judge_offset() const {
    return JudgeOffset;
}

double PlayerContext::GetRate() const {
    return Parameters.Rate;
}

void PlayerContext::judge_lane(uint32_t Lane, double Time) {
    GearKeyEvent(Lane, true);

    if (!CanJudge())
        return;

    auto &Notes = ChartState.notes_time_ordered[Lane];

    auto Start = Notes.begin();
    auto End = Notes.end();

    auto threshold = (PlayerScoreKeeper->usesO2() ?
                      PlayerScoreKeeper->getJudgmentCutoffMS() :
                      (PlayerScoreKeeper->getJudgmentCutoffMS() / 1000.0));

    // Use this optimization when we can make sure vertical properly aligns up with time, as with ReleaseLane.
    //if (ChartState.IsNoteTimeSorted())
    {
        auto timeLower = (Time - threshold);
        auto timeHigher = (Time + threshold);

        auto note_start_before = [&](const RuntimeNoteHandle handle, const double time) {
            return ChartState.note_at(Lane, handle)->get_start_time() < time;
        };
        auto time_before_note_start = [&](const double time, const RuntimeNoteHandle handle) {
            return time < ChartState.note_at(Lane, handle)->get_start_time();
        };

        Start = std::lower_bound(Notes.begin(), Notes.end(), timeLower, note_start_before);
        End = std::upper_bound(Notes.begin(), Notes.end(), timeHigher, time_before_note_start);
    }

    Gear.ClosestNoteMS[Lane] = MsDisplayMargin;

    for (auto mp = Start; mp != End; ++mp) {
        auto m = ChartState.note_at(Lane, *mp);
        if (!m) continue;
        double dev = (Time - m->get_start_time()) * 1000;
        double tD = abs(dev);

        Gear.ClosestNoteMS[Lane] = std::min(tD, (double) Gear.ClosestNoteMS[Lane]);

        if (Gear.ClosestNoteMS[Lane] >= MsDisplayMargin)
            Gear.ClosestNoteMS[Lane] = 0;

        if (!m->is_judgable())
            continue;

        if (MechanicsSet->OnPressLane(Time, m, Lane)) {
            return; // we judged a note in this lane, so we're done.
        }
    }

    if (Gear.CurrentKeysounds[Lane])
        PlayKeysound(Gear.CurrentKeysounds[Lane]->get_sound());
}

bool PlayerContext::has_failed() const {
    return PlayerScoreKeeper->isStageFailed(GetCurrentGaugeType()) && !Parameters.NoFail;
}

bool PlayerContext::has_delayed_failure() const
{
    return PlayerScoreKeeper->hasDelayedFailure(GetCurrentGaugeType());
}

double PlayerContext::GetClosestNoteTime(int lane) const {
    if (lane >= 0 && lane < GetChannelCount())
        return Gear.ClosestNoteMS[lane];
    else
        return std::numeric_limits<double>::infinity();
}

void PlayerContext::SetUserMultiplier(float Multip) {
    Parameters.UserSpeedMultiplier = Multip;
}

void PlayerContext::set_playable_data(std::shared_ptr<otoworm::Chart> chart, double Drift) {
    CfgVar JudgeOffsetMS("JudgeOffsetMS");
    double DesiredDefaultSpeed = Configuration::GetSkinConfigf("DefaultSpeedUnits");
    rd::ESpeedType Type = (rd::ESpeedType) (int) Configuration::GetSkinConfigf("DefaultSpeedKind");

    CurrentChart = std::move(chart);
    this->Drift = Drift;
    JudgeOffset = JudgeOffsetMS / 1000.0;

    // this has to happen after the setup so we can use the effective parameters!
    // use data from the replay if one is loaded
    if (PlayerReplay->IsLoaded()) {
        Parameters = PlayerReplay->GetEffectiveParameters();
        DesiredDefaultSpeed = Parameters.UserSpeedMultiplier;

        // treat desired default speed as the actual multiplier
        Type = SPEEDTYPE_MULTIPLIER;

        PlayerReplay->AddPlaybackListener([this](Replay::Entry entry) {
            this->on_player_key_event(entry.Time + this->get_drift(), entry.Down, entry.Lane);
        });
    }

    auto d = Setup(Parameters, DesiredDefaultSpeed, Type, Drift, CurrentChart);
    ChartState = *d;
    ChartState.prepare_ordered_notes(); // Invalid ordered notes until we do this.
    delete d;

    SetupMechanics();

    // setupmechanics sets the effective gauge/etc so we have to do that first
    const auto transient = GetOtoTransient(CurrentChart);
    PlayerReplay->SetSongData(
            Parameters,
            Type,
            transient ? transient->file_hash : "",
            transient ? transient->index_in_file : -1
    );

    UnitsPerMeasure = UNITS_PER_MEASURE;
}

std::vector<AutoplaySound> PlayerContext::GetBgmData() {
    CfgVar DisableKeysounds("DisableKeysounds");

    // Load up BGM events
    std::vector<AutoplaySound> BGMs;
    if (const auto transient = GetOtoTransient(CurrentChart)) {
        BGMs.reserve(transient->bgm_events.size());
        for (const auto& bgm : transient->bgm_events)
            BGMs.push_back(ToRdAutoplaySound(bgm));
    }

    if (DisableKeysounds)
        NoteTransform::MoveKeysoundsToBGM(CurrentChart->channels, ChartState.notes, BGMs, Drift);

    return BGMs;
}

double PlayerContext::has_song_finished(double time) const {
    double wt = ChartState.real_to_warped_time(time);
    double cutoff;

    if (PlayerScoreKeeper->usesO2()) { // beat-based judgements
        double curBPS = ChartState.get_bps_at(time);
        double cutoffspb = 1 / curBPS;

        cutoff = cutoffspb * PlayerScoreKeeper->getLateMissCutoffMS();
    } else // time-based judgments
        cutoff = PlayerScoreKeeper->getLateMissCutoffMS() / 1000.0;

    return wt > CurrentChart->duration + cutoff;
}

double PlayerContext::get_waiting_time() const
{
    CfgVar WaitingTime("WaitingTime");
    return std::max(std::max(WaitingTime > 1.0 ? WaitingTime : 1.5, 0.0), CurrentChart ? -CurrentChart->offset : 0);
}


void PlayerContext::GearKeyEvent(uint32_t Lane, bool KeyDown) const {
    if (OnGearKeyEvent) {
        OnGearKeyEvent(Lane, KeyDown, PlayerNumber);
    }
}

void PlayerContext::update(double songTime) {
    auto driftedTime = songTime - Drift;
    auto Beat = ChartState.get_beat_at(ChartState.real_to_warped_time(driftedTime));
    PlayerNoteskin->Update(songTime - LastUpdateTime, Beat);
    LastUpdateTime = songTime;
    PlayerReplay->Update(songTime - Drift);
    RunMeasures(songTime - Drift);
}

void PlayerContext::render(double SongTime) {
    int rnc = DrawMeasures(SongTime - Drift);

}

double PlayerContext::get_life_pst() const {
    auto LifebarType = GetCurrentGaugeType();
    auto lifebar_amount = PlayerScoreKeeper->getLifebarAmount(LifebarType);
    if (LifebarType == LT_GROOVE || LifebarType == LT_EASY)
        return std::max(2, int(floor(lifebar_amount * 50) * 2));
    else
        return ceil(lifebar_amount * 50) * 2;
}

std::string PlayerContext::GetPacemakerText(bool bm) const {
    if (bm) {
        auto bmpm = PlayerScoreKeeper->getAutoPacemaker();
        return bmpm.first;
    } else {
        auto pm = PlayerScoreKeeper->getAutoRankPacemaker();
        return pm.first;
    }
}

int PlayerContext::GetPacemakerValue(bool bm) const {
    if (bm) {
        auto bmpm = PlayerScoreKeeper->getAutoPacemaker();
        return bmpm.second;
    } else {
        auto pm = PlayerScoreKeeper->getAutoRankPacemaker();
        return pm.second;
    }
}

void PlayerContext::DrawBarlines(double CurrentVertical, double UserSpeedMultiplier) const
{
    for (const auto i : ChartState.barlines) {
        const double real_v = (CurrentVertical - i * UnitsPerMeasure) * UserSpeedMultiplier +
                       PlayerNoteskin->GetBarlineOffset() * sign(UserSpeedMultiplier) + GetJudgmentY();
        if (real_v > 0 && real_v < ScreenWidth) {
            Barline->SetLocation(Vec2(PlayerNoteskin->GetBarlineStartX(), real_v),
                                 Vec2(PlayerNoteskin->GetBarlineStartX() + PlayerNoteskin->GetBarlineWidth(), real_v));
            Barline->Render();
        }
    }
}

Mat4 id;

int PlayerContext::DrawMeasures(double song_time) {
    int rnc = 0;
    /*
        DrawMeasures should get the unwarped song time.
        Internally, it uses warped song time.
    */
    auto wt = ChartState.real_to_warped_time(song_time);

    // note Y displacement at song_time
    auto chart_displacement = ChartState.get_displacement_at(wt) * UnitsPerMeasure;

    // effective speed multiplier
    auto chart_multiplier = GetAppliedSpeedMultiplier(song_time);
    auto effective_chart_speed_multiplier = chart_multiplier * Parameters.UserSpeedMultiplier;

    // since + is downward, - is upward!
    bool upscrolling = effective_chart_speed_multiplier < 0;

    if (PlayerNoteskin->IsBarlineEnabled())
        DrawBarlines(chart_displacement, effective_chart_speed_multiplier);

    // Set some parameters...
    renderer::set_shader_parameters(false, true, false, false, Parameters.GetHiddenMode());

    // Sudden = 1, Hidden = 2, flashlight = 3 (Defined in the shader)
    if (Parameters.GetHiddenMode()) {
        renderer::Shader::SetUniform(
                renderer::DefaultShader::GetUniform(renderer::U_HIDCENTER),
                Parameters.GetHiddenCenter());
        renderer::Shader::SetUniform(
                renderer::DefaultShader::GetUniform(renderer::U_HIDSIZE),
                Parameters.GetHiddenTransitionSize());
        renderer::Shader::SetUniform(
                renderer::DefaultShader::GetUniform(renderer::U_HIDFLSIZE),
                Parameters.GetHiddenCenterSize());
    }

    renderer::set_primitive_quad_vbo();
    auto &Notes = ChartState.notes_vertically_ordered;
    auto jy = GetJudgmentY();

    for (auto k = 0U; k < CurrentChart->channels; k++) {
        // From the note's vertical StaticVert transform to position on screen.
        auto Locate = [&](double StaticVert) -> double {
            return (chart_displacement - StaticVert * UnitsPerMeasure) * effective_chart_speed_multiplier + jy;
        };

        auto Start = Notes[k].begin();
        auto End = Notes[k].end();

        // We've got guarantees about our note locations.
        //if (ChartState.IsNoteTimeSorted())
        {
            /* Find the location of the first/next visible regular note */
            auto LocPredicate = [&](const RuntimeNoteHandle handle, double TrackDisplacement) -> bool {
                auto* A = ChartState.note_at(k, handle);
                if (!upscrolling)
                    return TrackDisplacement < Locate(A->get_vertical());
                else // Signs are switched. We need to preserve the same order.
                    return TrackDisplacement > Locate(A->get_vertical());
            };

            // Signs are switched. Doesn't begin by the first note closest to the lower edge, but the one closest to the higher edge.
            if (!upscrolling)
                Start = std::lower_bound(
                        Notes[k].begin(),
                        Notes[k].end(),
                        ScreenHeight + PlayerNoteskin->GetNoteOffset(),
                        LocPredicate);
            else
                Start = std::lower_bound(
                        Notes[k].begin(),
                        Notes[k].end(),
                        0 - PlayerNoteskin->GetNoteOffset(),
                        LocPredicate);

            // Locate the first hold that we can draw in this range
            /*
                Since our object is on screen, our hold may be on screen but may not be this note
                since only head locations are used.
                Find this possible hold by checking if it intersects the screen.
            */
            if (Start != Notes[k].begin()) {
                auto i = Start - 1;//std::reverse_iterator<std::vector<TrackNote>::iterator>(Start);
                auto* note = ChartState.note_at(k, *i);
                if (note && note->is_hold() && note->is_visible()) {
                    auto Vert = Locate(note->get_vertical());
                    auto VertEnd = Locate(note->get_hold_end_vertical());
                    if (IntervalsIntersect(0, ScreenHeight, std::min(Vert, VertEnd), std::max(Vert, VertEnd))) {
                        Start = i;
                    }
                }
            }

            // Find the note that is out of the drawing range
            // As before. Top becomes bottom, bottom becomes top.
            if (!upscrolling)
                End = std::lower_bound(Notes[k].begin(), Notes[k].end(), 0 - PlayerNoteskin->GetNoteOffset(),
                                       LocPredicate);
            else
                End = std::lower_bound(Notes[k].begin(), Notes[k].end(),
                                       ScreenHeight + PlayerNoteskin->GetNoteOffset(), LocPredicate);
        }

        // Now, draw them.
        for (auto mp = Start; mp != End; ++mp) {
            auto m = ChartState.note_at(k, *mp);
            if (!m) continue;
            double Vertical = 0;
            double VerticalHoldEnd;

            // Don't attempt drawing this object if not visible.
            if (!m->is_visible())
                continue;

            Vertical = Locate(m->get_vertical());
            VerticalHoldEnd = Locate(m->get_hold_end_vertical());

            // Old check method that doesn't rely on a correct vertical ordering.
            /*if (!ChartState.IsNoteTimeSorted())
            {
                if (m->is_hold())
                {
                    if (!IntervalsIntersect(0, ScreenHeight,
                        std::min(Vertical, VerticalHoldEnd), std::max(Vertical, VerticalHoldEnd))) continue;
                }
                else
                {
                    if (Vertical < -PlayerNoteskin->GetNoteOffset() ||
                        Vertical > ScreenHeight + PlayerNoteskin->GetNoteOffset()) continue;
                }
            }*/

            double JudgeY;

            // LR2 style keep-on-the-judgment-line
            bool AboveLine = Vertical < GetJudgmentY();
            if (!(AboveLine ^ upscrolling) && m->is_judgable())
                JudgeY = GetJudgmentY();
            else
                JudgeY = Vertical;

            // We draw the body first, so that way the heads get drawn on top
            if (m->is_hold()) {
                // todo: move this note state determination to the note itself
                enum : int {
                    Failed, Active, BeingHit, SuccesfullyHit
                };
                int Level = -1;

                if (m->is_enabled() && !m->failed_hit())
                    Level = Active;
                if (!m->is_enabled() && m->failed_hit())
                    Level = Failed;
                if (!m->is_enabled() && !m->failed_hit() && !m->was_hit())
                    Level = Failed;
                if (m->is_enabled() && m->was_hit() && !m->failed_hit())
                    Level = BeingHit;
                if (!m->is_enabled() && m->was_hit() && !m->failed_hit())
                    Level = SuccesfullyHit;

                double Pos;
                double Size;
                // If we're being hit and..
                bool decrease_hold_size = PlayerNoteskin->ShouldDecreaseHoldSizeWhenBeingHit() && Level == 2;
                auto reference_point = 0.0f;
                if (decrease_hold_size) {
                    reference_point = JudgeY;
                } else // We were failed, not being hit or were already hit
                {
                    reference_point = Vertical;
                }

                Pos = (VerticalHoldEnd + reference_point) / 2;
                Size = VerticalHoldEnd - reference_point;

                PlayerNoteskin->DrawHoldBody(k, Pos, Size, Level);
                PlayerNoteskin->DrawHoldTail(*m, k, VerticalHoldEnd, Level);

                if (PlayerNoteskin->AllowDanglingHeads() || decrease_hold_size)
                    PlayerNoteskin->DrawHoldHead(*m, k, JudgeY, Level);
                else
                    PlayerNoteskin->DrawHoldHead(*m, k, Vertical, Level);
            } else {
                if (PlayerNoteskin->AllowDanglingHeads())
                    PlayerNoteskin->DrawNote(*m, k, JudgeY);
                else
                    PlayerNoteskin->DrawNote(*m, k, Vertical);
            }

            rnc++; // Rendered note count increases...
        }
    }

    /* Clean up */
    renderer::set_shader_parameters(false, true, false, false, 0);
    renderer::finalize_draw();


    if (DebugNoteRendering) {
        fnt->render(Utility::Format(
                "NOTES RENDERED: %d\nSORTEDTIME: %d\nRNG: %f to %f\nMULT/EFFECTIVEMULT/SPEED: %f/%f/%f",
                rnc,
                true,//ChartState.IsNoteTimeSorted(),
                chart_displacement, chart_displacement + ScreenHeight,
                chart_multiplier, effective_chart_speed_multiplier,
                get_current_vertical_speed() * effective_chart_speed_multiplier), Vec2(0, 0));
    }
    return rnc;
}
