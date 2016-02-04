#include "pch.h"

#include "GameGlobal.h"
#include "Screen.h"
#include "Audio.h"
#include "ImageLoader.h"
#include "GameWindow.h"

#include "LuaManager.h"
#include "Sprite.h"
#include "SceneEnvironment.h"
#include "ImageList.h"

#include "ScoreKeeper7K.h"
#include "ScreenGameplay7K.h"
#include "ScreenGameplay7K_Mechanics.h"

//#include <glm/gtc/matrix_transform.inl>

using namespace VSRG;

void ScreenGameplay7K::RecalculateMatrix()
{
    PositionMatrix = glm::translate(Mat4(), glm::vec3(0, JudgmentLinePos + CurrentVertical * SpeedMultiplier, 0));
}

void ScreenGameplay7K::RecalculateEffects()
{
    if (waveEffectEnabled)
    {
        float cs = sin(CurrentBeat * M_PI / 4);
        waveEffect = cs * 0.35 * std::min(SpeedMultiplierUser, 2.0f);
        MultiplierChanged = true;
    }

    // Calculate current speed value to apply.
    auto CurrentTime = GetWarpedSongTime();
    auto speedIter = lower_bound(Speeds.begin(), Speeds.end(), CurrentTime);
    double previousValue = 1;
    double currentValue = 1;
    double speedTime = CurrentTime;
    double duration = 1;

    // The result of lower_bound comes after the current speed to apply.
    if (speedIter != Speeds.begin())
        --speedIter;

    // Do we have a speed that has a value to interpolate from?
    if (speedIter != Speeds.begin())
    {
        auto prevSpeed = speedIter - 1;
        previousValue = prevSpeed->Value;
        currentValue = speedIter->Value;
        duration = speedIter->Duration;
        speedTime = speedIter->Time;
    }
    else // Oh, we don't?
    {
        if (speedIter != Speeds.end())
            currentValue = previousValue = speedIter->Value;
    }

    auto speedProgress = Clamp((CurrentTime - speedTime) / duration, 0.0, 1.0);
    auto lerpedMultiplier = speedProgress * (currentValue - previousValue) + previousValue;

    if (Upscroll)
        SpeedMultiplier = -(SpeedMultiplierUser + waveEffect + beatScrollEffect);
    else
        SpeedMultiplier = SpeedMultiplierUser + waveEffect + beatScrollEffect;

    SpeedMultiplier *= lerpedMultiplier;
}

void ScreenGameplay7K::HitNote(double TimeOff, uint32_t Lane, bool IsHold, bool IsHoldRelease)
{
    int Judgment = ScoreKeeper->hitNote(TimeOff);

    UpdateScriptScoreVariables();

    if (Animations->GetEnv()->CallFunction("HitEvent", 5))
    {
        Animations->GetEnv()->PushArgument(Judgment);
        Animations->GetEnv()->PushArgument(TimeOff);
        Animations->GetEnv()->PushArgument((int)Lane + 1);
        Animations->GetEnv()->PushArgument(IsHold);
        Animations->GetEnv()->PushArgument(IsHoldRelease);
        Animations->GetEnv()->RunFunction();
    }

    BGA->OnHit();

    if (ScoreKeeper->getMaxNotes() == ScoreKeeper->getScore(ST_NOTES_HIT))
        Animations->DoEvent("OnFullComboEvent");
}

void ScreenGameplay7K::MissNote(double TimeOff, uint32_t Lane, bool IsHold, bool auto_hold_miss, bool early_miss)
{
    ScoreKeeper->missNote(auto_hold_miss, early_miss);

    if (IsHold)
        HeldKey[Lane] = false;

    // per-theme seconds showing miss bga
    MissTime = Configuration::GetSkinConfigf("OnMissBGATime");

    UpdateScriptScoreVariables();

    if (Animations->GetEnv()->CallFunction("MissEvent", 3))
    {
        Animations->GetEnv()->PushArgument(TimeOff);
        Animations->GetEnv()->PushArgument((int)Lane + 1);
        Animations->GetEnv()->PushArgument(IsHold);
        Animations->GetEnv()->RunFunction();
    }

    BGA->OnMiss();
}

void ScreenGameplay7K::PlayLaneKeysound(uint32_t Lane)
{
    TrackNote *TN = CurrentKeysounds[Lane];
    if (!TN) return;

    if (Keysounds.find(TN->GetSound()) != Keysounds.end() && PlayReactiveSounds)
        for (auto &&s : Keysounds[TN->GetSound()])
            if (s) s->Play();
}

void ScreenGameplay7K::PlayKeysound(TrackNote* TN)
{
    if (Keysounds.find(TN->GetSound()) != Keysounds.end() && PlayReactiveSounds)
        for (auto &&s : Keysounds[TN->GetSound()])
            if (s) s->Play();
}

void ScreenGameplay7K::SetLaneHoldState(uint32_t Lane, bool NewState)
{
    HeldKey[Lane] = NewState;
}

// true if holding down key
bool ScreenGameplay7K::GetGearLaneState(uint32_t Lane)
{
    return GearIsPressed[Lane] != 0;
}
void ScreenGameplay7K::RunMeasures()
{
    double timeClosest[VSRG::MAX_CHANNELS];

    for (int i = 0; i < VSRG::MAX_CHANNELS; i++)
        timeClosest[i] = CurrentDiff->Duration;

    double usedTime = GetSongTime();

    for (auto k = 0U; k < CurrentDiff->Channels; k++)
    {
        for (auto m = NotesByChannel[k].begin(); m != NotesByChannel[k].end(); ++m)
        {
            if (!m->IsJudgable())
                continue;

            // Keysound update to closest note.
            if (m->IsEnabled() && m->IsJudgable())
            {
                if ((abs(usedTime - m->GetTimeFinal()) < timeClosest[k]))
                {
                    if (CurrentDiff->IsVirtual)
                        CurrentKeysounds[k] = &(*m);
                    timeClosest[k] = abs(usedTime - m->GetTimeFinal());
                }
                else
                    break; // In other words, we're getting further away.
            }

            // Autoplay
            if (Auto)
            {
                double TimeThreshold = usedTime + 0.008; // latest time a note can activate.
                if (m->GetStartTime() <= TimeThreshold)
                {
                    if (m->IsEnabled())
                    {
                        if (m->IsHold())
                        {
                            if (m->WasNoteHit())
                            {
                                if (m->GetTimeFinal() < TimeThreshold)
                                {
                                    double hit_time = clamp_to_interval(usedTime, m->GetTimeFinal(), 0.008);
                                    // We use clamp_to_interval for those pesky outliers.
                                    if (perfect_auto) ReleaseLane(k, m->GetTimeFinal());
                                    else ReleaseLane(k, hit_time);
                                }
                            }
                            else
                            {
                                double hit_time = clamp_to_interval(usedTime, m->GetStartTime(), 0.008);
                                if (perfect_auto) JudgeLane(k, m->GetStartTime());
                                else JudgeLane(k, hit_time);
                            }
                        }
                        else
                        {
                            double hit_time = clamp_to_interval(usedTime, m->GetStartTime(), 0.008);
                            if (perfect_auto)
                            {
                                JudgeLane(k, m->GetStartTime());
                                ReleaseLane(k, m->GetTimeFinal());
                            }
                            else
                            {
                                JudgeLane(k, hit_time);
                                ReleaseLane(k, hit_time);
                            }
                        }
                    }
                }
            }

            if (stage_failed) continue; // don't check for judgments after stage has failed.

            if (MechanicsSet->OnUpdate(usedTime, &(*m), k))
                break;
        } // end for notes
    } // end for channels
}

void ScreenGameplay7K::ReleaseLane(uint32_t Lane, float Time)
{
    GearKeyEvent(Lane, false);

    if (stage_failed) return; // don't judge any more after stage is failed.

    auto Start = NotesByChannel[Lane].begin();
    auto End = NotesByChannel[Lane].end();

    // Use this optimization when we can make sure vertical properly aligns up with time.
    if (!Warps.size() && !HasNegativeScroll)
    {
        // In comparison to the regular compare function, since end times are what matter with holds (or lift events, where start == end)
        // this does the job as it should instead of comparing start times where hold tails would be completely ignored.
        auto LboundFunc = [](const TrackNote &A, const double &B) -> bool
        {
            return A.GetTimeFinal() < B;
        };
        auto HboundFunc = [](const double &A, const TrackNote &B) -> bool
        {
            return A < B.GetTimeFinal();
        };

        auto timeLower = (Time - (ScoreKeeper->usesO2() ? ScoreKeeper->getMissCutoff() : (ScoreKeeper->getMissCutoff() / 1000.0)));
        auto timeHigher = (Time + (ScoreKeeper->usesO2() ? ScoreKeeper->getEarlyMissCutoff() : (ScoreKeeper->getEarlyMissCutoff() / 1000.0)));

        Start = std::lower_bound(NotesByChannel[Lane].begin(), NotesByChannel[Lane].end(), timeLower, LboundFunc);

        // Locate the first hold that we can judge in this range (Pending holds. Similar to what was done when drawing.)
        auto rStart = std::reverse_iterator<std::vector<TrackNote>::iterator>(Start);
        for (auto i = rStart; i != NotesByChannel[Lane].rend(); ++i)
        {
            if (i->IsHold() && i->IsEnabled() && i->IsJudgable() && i->WasNoteHit() && !i->FailedHit())
                Start = i.base() - 1;
        }

        End = std::upper_bound(NotesByChannel[Lane].begin(), NotesByChannel[Lane].end(), timeHigher, HboundFunc);

        if (End != NotesByChannel[Lane].end())
            ++End;
    }

    for (auto m = Start; m != End; ++m)
    {
        if (!m->IsJudgable()) continue;
        if (MechanicsSet->OnReleaseLane(Time, &(*m), Lane)) // Are we done judging..?
            break;
    }
}

void ScreenGameplay7K::JudgeLane(uint32_t Lane, float Time)
{
    GearKeyEvent(Lane, true);

    if ((!Music && !CurrentDiff->IsVirtual) || !Active || stage_failed)
        return;

    auto Start = NotesByChannel[Lane].begin();
    auto End = NotesByChannel[Lane].end();

    // Use this optimization when we can make sure vertical properly aligns up with time, as with ReleaseLane.
    if (!Warps.size() && !HasNegativeScroll)
    {
        auto timeLower = (Time - (ScoreKeeper->usesO2() ? ScoreKeeper->getMissCutoff() : (ScoreKeeper->getMissCutoff() / 1000.0)));
        auto timeHigher = (Time + (ScoreKeeper->usesO2() ? ScoreKeeper->getEarlyMissCutoff() : (ScoreKeeper->getEarlyMissCutoff() / 1000.0)));

        Start = std::lower_bound(NotesByChannel[Lane].begin(), NotesByChannel[Lane].end(), timeLower);
        End = std::upper_bound(NotesByChannel[Lane].begin(), NotesByChannel[Lane].end(), timeHigher);
    }

    bool notJudged = true;

    lastClosest[Lane] = MsDisplayMargin;

    for (auto m = Start; m != End; ++m)
    {
        double dev = (Time - m->GetStartTime()) * 1000;
        double tD = abs(dev);

        lastClosest[Lane] = std::min(tD, (double)lastClosest[Lane]);

        if (lastClosest[Lane] >= MsDisplayMargin)
            lastClosest[Lane] = 0;

        if (!m->IsJudgable())
            continue;

        if (MechanicsSet->OnPressLane(Time, &(*m), Lane))
        {
            notJudged = false;
            return; // we judged a note in this lane, so we're done.
        }
    }

    if (notJudged)
        PlayLaneKeysound(Lane);
}