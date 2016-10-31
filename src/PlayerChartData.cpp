#include "pch.h"
#include "Song7K.h"
#include "PlayerChartData.h"
#include "Logging.h"

CfgVar DebugMeasurePosGen("MeasurePosGen", "Debug");

namespace Game {
	namespace VSRG {
		PlayerChartData::PlayerChartData()
		{
			Drift = 0;
			WaitTime = DEFAULT_WAIT_TIME;
			HasNegativeScroll = false;
			HasTurntable = false;
			ConnectedDifficulty = nullptr;
		}

		// If TimingType is Beat, use beat integration to get time in seconds at Time
		// Otherwise just sum it and stuff.
		double TimeFromTimingKind(const TimingData &Timing,
			const TimingData &StopsTiming,
			const double Time,
			VSRG::Difficulty::ETimingType TimingType,
			double Offset,
			double Drift)
		{
			if (TimingType == VSRG::Difficulty::BT_BEAT) // Time is in Beats
			{
				return TimeAtBeat(Timing, Drift + Offset, Time) + StopTimeAtBeat(StopsTiming, Time);
			}
			else if (TimingType == VSRG::Difficulty::BT_MS || TimingType == VSRG::Difficulty::BT_BEATSPACE) // Time is in MS
			{
				return Time + Drift + Offset;
			}

			assert(0); // Never happens. Must never happen ever ever.
			return 0;
		}

		// Get the BPS depending on the timing type.
		double BPSFromTimingKind(double Value, VSRG::Difficulty::ETimingType TimingType)
		{
			if (TimingType == VSRG::Difficulty::BT_BEAT || TimingType == VSRG::Difficulty::BT_MS) // Time is in Beats
			{
				return bps(Value);
			}

			if (TimingType == VSRG::Difficulty::BT_BEATSPACE) // Time in MS, and not using bpm, but ms per beat.
			{
				return bps(60000.0 / Value);
			}

			assert(0);
			return 0; // shut up compiler
		}

		TimingData GetBPSData(VSRG::Difficulty *difficulty, double Drift)
		{
			const auto &data = difficulty->Data;
			const auto &Timing = difficulty->Timing;
			auto Offset = difficulty->Offset;
			auto BPMType = difficulty->BPMType;
			/*
				Calculate BPS.
				BPS time is calculated applying the offset and drift.
			*/
			assert(data != NULL);

			auto &StopsTiming = data->Stops;
			TimingData BPS;

			/*
				We want to get the time for every bpm timing segment.
			*/
			for (auto Time : Timing)
			{
				TimingSegment Seg;

				Seg.Time = TimeFromTimingKind(Timing, StopsTiming, Time.Time, BPMType, Offset, Drift);
				Seg.Value = BPSFromTimingKind(Time.Value, BPMType);

				BPS.push_back(Seg);
			}

			if (!StopsTiming.size() || BPMType != VSRG::Difficulty::BT_BEAT) // Stops only supported in Beat mode.
				return BPS;

			/*
				Here on, just working with stops.
				We want to put them as a BPS 0 event.
			*/
			for (auto Time = StopsTiming.begin();
			Time != StopsTiming.end();
				++Time)
			{
				TimingSegment Seg;
				double StopStartTime = TimeAtBeat(Timing, Offset + Drift, Time->Time) + StopTimeAtBeat(StopsTiming, Time->Time);
				double StopEndTime = StopStartTime + Time->Value;

				/* Initial Stop */
				Seg.Time = StopStartTime;
				Seg.Value = 0;

				/* First, eliminate collisions. */
				for (auto k = BPS.begin(); k != BPS.end();)
				{
					/*
						Equal? Remove the collision, leaving only the 0 in front.
						There's no "close enough" here to matter.
						They're all acting on the same precision and it's not lost at any time.
						Both times are equal if they r
					*/
					if (k->Time == StopStartTime)
					{
						k = BPS.erase(k);

						if (k == BPS.end())
							break;
						else continue; // In the strange case there's overlap of BPM right here.
					}

					++k;
				}

				// Okay, the collision is out. Let's push our 0-speeder.
				BPS.push_back(Seg);

				// Now we find what bps to restore to.
				auto bpsRestore = bps(SectionValue(Timing, Time->Time));

				for (auto k = BPS.begin(); k != BPS.end(); )
				{
					// There's BPM changes in between the stop?
					if (k->Time > StopStartTime && k->Time <= StopEndTime)
					{
						bpsRestore = k->Value; /* This is the last speed change in the interval that the stop lasts. We'll use it. */

						/* Eliminate this since we're not going to use it. Override with a stop, in other words. */
						k = BPS.erase(k);

						if (k == BPS.end())
							break;
						continue;
					}

					++k;
				}

				/*
					Restored speed after stop
					Since we use <= StopEndTime slightly back
					it'll get removed if for some reason something overlaps
					with this stop's end time
					It's fine though, we recorded that last thingy's BPS value.
					Let's just hope it wasn't a stop. Though that'd be silly.
				*/
				Seg.Time = StopEndTime;
				Seg.Value = bpsRestore;
				BPS.push_back(Seg);
			}

			std::sort(BPS.begin(), BPS.end());
			return BPS;
		}

		TimingData GetVSpeeds(TimingData& BPS, double Speed)
		{
			TimingData VerticalSpeeds;

			// We're using a CMod, so further processing is pointless
			if (Speed)
			{
				VerticalSpeeds.push_back(TimingSegment(0, Speed));
				return VerticalSpeeds;
			}
			// End CMod

			// Calculate velocity at time based on BPM at time
			for (auto Section : BPS)
			{
				double VerticalSpeed;
				if (Section.Value)
				{
					auto spb = 1 / Section.Value;
					VerticalSpeed = MeasureBaseSpacing / (spb * 4);
				}
				else
					VerticalSpeed = 0;

				// We blindly take the BPS time that had offset and drift applied.
				VerticalSpeeds.push_back(TimingSegment(Section.Time, VerticalSpeed));
			}

			// Let first speed be not-null.
			if (VerticalSpeeds.size() && VerticalSpeeds[0].Value == 0)
			{
				for (auto i = VerticalSpeeds.begin();
				i != VerticalSpeeds.end();
					++i)
				{
					if (i->Value != 0)
						VerticalSpeeds[0].Value = i->Value;
				}
			}

			return VerticalSpeeds;
		}

		TimingData GetSpeedChanges(TimingData VerticalSpeeds, TimingData Scrolls, double Drift, double Offset, bool Reset)
		{
			std::sort(Scrolls.begin(), Scrolls.end());

			const auto Unmodified = VerticalSpeeds;

			for (TimingData::const_iterator Change = Scrolls.begin();
			Change != Scrolls.end();
				++Change)
			{
				TimingData::const_iterator NextChange = (Change + 1);
				double ChangeTime = Change->Time + Drift + Offset;

				/*
					Find all 
					if there exists a speed change which is virtually happening at the same time as this VSpeed
					modify it to be this value * factor
				*/

				bool insert = true;
				for (auto Time = VerticalSpeeds.begin();
				Time != VerticalSpeeds.end();
					++Time)
				{
					if (abs(ChangeTime - Time->Time) < 0.00001)
					{
						Time->Value *= Change->Value;
						insert = false;
					}
				}


				/*
					There are no collisions- insert a new speed at this time
				*/

				if (insert) {
					if (ChangeTime < 0)
						continue;

					auto SpeedValue = SectionValue(Unmodified, ChangeTime) * Change->Value;

					TimingSegment VSpeed;

					VSpeed.Time = ChangeTime;
					VSpeed.Value = SpeedValue;

					VerticalSpeeds.push_back(VSpeed);
				}

				/*
					Theorically, if there were a VSpeed change after this one (such as a BPM change) we've got to modify them
					if they're between this and the next speed change.

					Apparently, this behaviour is a "bug" since osu!mania resets SV changes
					after a BPM change.
				*/

				if (Reset) // Okay, we're an osu!mania chart, leave the resetting.
					continue;

				// We're not an osu!mania chart, so it's time to do what should be done.
				// All VSpeeds with T > current and T < next is a BPM change speed;
				// multiply it by the value of the current speed
				for (auto Time = VerticalSpeeds.begin();
				Time != VerticalSpeeds.end();
					++Time)
				{
					if (Time->Time > ChangeTime)
					{
						// Two options, between two speed changes, or the last one. Second case, NextChange == Scrolls.end().
						// Otherwise, just move on
						// Last speed change
						if (NextChange == Scrolls.end())
						{
							Time->Value = Change->Value * SectionValue(Unmodified, Time->Time);
						}
						else
						{
							if (Time->Time < NextChange->Time) // Between speed changes
								Time->Value = Change->Value * SectionValue(Unmodified, Time->Time);
						}
					}
				}
			}

			std::sort(VerticalSpeeds.begin(), VerticalSpeeds.end());
			return VerticalSpeeds;
		}

		double PlayerChartData::GetWarpAmount(double Time) const
		{
			double wAmt = 0;
			for (auto warp : Warps)
			{
				if (warp.Time < Time)
					wAmt += warp.Value;
			}

			return wAmt;
		}

		bool PlayerChartData::IsWarpingAt(double start_time) const
		{
			auto it = std::lower_bound(Warps.begin(), Warps.end(), start_time, TimeSegmentCompare<TimingSegment>);
			if (it != Warps.end())
				return it->Time + it->Value > start_time;
			else
				return false;
		}


		PlayerChartData PlayerChartData::FromDifficulty(Difficulty *diff, double Drift, double Speed)
		{
			PlayerChartData out;
			auto data = diff->Data;
			if (data == nullptr)
				throw std::runtime_error("Tried to pass a metadata-only difficulty to Player Chart data generator.\n");

			out.ConnectedDifficulty = diff;
			out.Drift = Drift;
			out.BPS = GetBPSData(diff, Drift);
			out.VSpeeds = GetVSpeeds(out.BPS, Speed);
			out.VSpeeds = GetSpeedChanges(out.VSpeeds, data->Scrolls,
				Drift, diff->Offset,
				diff->BPMType == VSRG::Difficulty::BT_BEATSPACE);


			out.HasTurntable = diff->Data->Turntable;

			out.MeasureBarlines = out.GetMeasureLines();

			if (!Speed) {
				out.Warps = data->Warps;
				out.Speeds = data->Speeds;
			}

			/* For all channels of this difficulty */
			for (int KeyIndex = 0; KeyIndex < diff->Channels; KeyIndex++)
			{
				int MIdx = 0;
				auto MsrBeat = 0.0;

				/* For each measure of this channel */
				for (auto Msr : data->Measures)
				{
					/* For each note in the measure... */
					auto total_notes = Msr.Notes[KeyIndex].size();

					for (auto CurrentNote: Msr.Notes[KeyIndex])
					{
						/*
							Calculate position. (Change this to TrackNote instead of processing?)
							issue is not having the speed change data there.
						*/
						TrackNote NewNote;

						NewNote.AssignNotedata(CurrentNote);
						NewNote.AddTime(Drift);

						auto VerticalPosition = IntegrateToTime(out.VSpeeds, NewNote.GetStartTime());
						auto HoldEndPosition = IntegrateToTime(out.VSpeeds, NewNote.GetTimeFinal());

						// if upscroll change minus for plus as well as matrix at screengameplay7k
						if (!CurrentNote.EndTime)
							NewNote.AssignPosition(VerticalPosition);
						else
							NewNote.AssignPosition(VerticalPosition, HoldEndPosition);

						// Okay, now we want to know what fraction of a beat we're dealing with
						// this way we can display colored (a la Stepmania) notes.
						// We should do this before changing time by drift.
						double NoteBeat = IntegrateToTime(out.BPS, NewNote.GetStartTime());
						double dBeat = NoteBeat - MsrBeat; // do in relation to position from start of measure
						double BeatFraction = dBeat - floor(dBeat);

						NewNote.AssignFraction(BeatFraction);

						// Notes use warped time. Unwarp it.
						double Wamt = -out.GetWarpAmount(CurrentNote.StartTime);
						NewNote.AddTime(Wamt);

						// !Speed: non-constant
						// Judgable & ! warping: Constant speed, so only add non-warped notes.
						if (!Speed || (NewNote.IsJudgable() && !out.IsWarpingAt(CurrentNote.StartTime)))
							out.NotesByChannel[KeyIndex].push_back(NewNote);
					}

					MIdx++;
					MsrBeat += Msr.Length;
				}

				// done with the channel - sort it
				std::stable_sort(out.NotesByChannel[KeyIndex].begin(), out.NotesByChannel[KeyIndex].end(),
					[](const TrackNote &A, const TrackNote &B) -> bool
				{
					return A.GetVertical() < B.GetVertical();
				});
			}

			if (Speed) // Only apply speeds on non-constant velocities
				out.Speeds = diff->Data->Speeds;

			for (auto&& w : out.Warps)
				w.Time += Drift;
			for (auto&& s : out.Speeds)
				s.Time += Drift;

			// Toggle whether we can use our guarantees for optimizations or not at rendering/judge time.
			out.HasNegativeScroll = false;

			for (auto S : out.Speeds) if (S.Value < 0) out.HasNegativeScroll = true;
			for (auto S : out.VSpeeds) if (S.Value < 0) out.HasNegativeScroll = true;
			return out;
		}

		double PlayerChartData::GetWarpedSongTime(double SongTime) const
		{
			auto T = SongTime;
			for (auto k = Warps.cbegin(); k != Warps.cend(); ++k)
			{
				if (k->Time <= T)
					T += k->Value;
			}

			return T;
		}

		TimingData BPStoSPB(TimingData BPS)
		{
			auto BPSCopy = BPS;
			for (auto &&i : BPS)
			{
				i.Value = 1.0 / i.Value;
				i.Time = IntegrateToTime(BPSCopy, i.Time); // Find time in beats based off beats in time
			}

			return BPS;
		}


		std::vector<double> PlayerChartData::GetMeasureLines() const
		{
			auto &diff = ConnectedDifficulty;
			auto Data = diff->Data;
			auto Timing = diff->Timing;
			auto BPMType = diff->BPMType;
			double Last = 0;
			TimingData SPB = BPStoSPB(BPS);


			std::vector<double> Out;

			if (!Data)
				return Out;

			if (!Timing.size())
				return Out;

			if (!Data->Measures.size())
				return Out;


			// Add lines before offset, and during waiting time...
			double BPS = BPSFromTimingKind(Timing[0].Value, diff->BPMType);
			double PreTime = WaitTime + diff->Offset + Drift;
			double PreTimeBeats = BPS * PreTime;
			int TotMeasures = PreTimeBeats / Data->Measures[0].Length;
			double MeasureTime = 1 / BPS * Data->Measures[0].Length;

			if (DebugMeasurePosGen)
				Log::LogPrintf("Total pre-offset measures: %d (pt: %f, ptb: %f, mtime: %f)\n", TotMeasures, PreTime, PreTimeBeats, MeasureTime);

			for (auto i = 0; i < TotMeasures; i++)
			{
				auto T = Drift + diff->Offset - MeasureTime * i;
				auto PositionOut = IntegrateToTime(VSpeeds, T);
				Out.push_back(PositionOut);
				if (DebugMeasurePosGen)
					Log::LogPrintf("Add measure line at time %f (Vertical %f)\n", T, PositionOut);
			}

			// Add
			for (auto Msr : Data->Measures)
			{
				double PositionOut = 0.0;

				if (BPMType == Difficulty::BT_BEAT) // VerticalSpeeds already has drift applied, so we don't need to apply it again here.
				{
					PositionOut = IntegrateToTime(VSpeeds, Drift + GetTimeAtBeat(Last));
				}
				else if (BPMType == Difficulty::BT_BEATSPACE)
				{
					auto TargetTime = IntegrateToTime(SPB, Last) + diff->Offset;
					//TargetTime = round(TargetTime * 1000.0) / 1000.0; // Round to MS

					PositionOut = IntegrateToTime(VSpeeds, TargetTime);
					if (DebugMeasurePosGen) {
						Log::LogPrintf("Add measure line at time %f (Vertical: %f)\n", TargetTime, PositionOut);
					}
				}

				Out.push_back(PositionOut);
				Last += Msr.Length;
			}

			return Out;
		}

		double PlayerChartData::GetBpmAt(double Time) const
		{
			return SectionValue(BPS, Time) * 60;
		}

		double PlayerChartData::GetBpsAt(double Time) const
		{
			return SectionValue(BPS, Time);
		}


		double PlayerChartData::GetBeatAt(double Time) const
		{
			return IntegrateToTime(BPS, Time);
		}

		double PlayerChartData::GetSpeedMultiplierAt(double Time) const
		{
			// Calculate current speed value to apply.
			auto CurrentTime = GetWarpedSongTime(Time);

			// speedIter: first which time is greater than current
			auto speedIter = lower_bound(Speeds.begin(), Speeds.end(), CurrentTime);
			double previousValue = 1;
			double currentValue = 1;
			double speedTime = CurrentTime;
			double duration = 1;
			bool integrateByBeats = false;

			if (speedIter != Speeds.begin())
				speedIter--;

			// Do we have a speed that has a value to interpolate from?
			/*
				Elaboration:
				From the speed at the end of the previous speed, interpolate to speedIter->value
				in speedIter->duration time across the current time - speedIter->time.
			*/
			if (speedIter != Speeds.begin())
			{
				auto prevSpeed = speedIter - 1;
				previousValue = prevSpeed->Value;
				currentValue = speedIter->Value;
				duration = speedIter->Duration;
				speedTime = speedIter->Time;
				integrateByBeats = speedIter->IntegrateByBeats;
			}
			else // Oh, we don't?
			{
				if (speedIter != Speeds.end())
					currentValue = previousValue = speedIter->Value;
			}

			double speedProgress;
			if (!integrateByBeats)
				speedProgress = Clamp((CurrentTime - speedTime) / duration, 0.0, 1.0);
			else {
				// Assume duration in beats if IntegrateByBeats is true
				speedProgress = Clamp((GetBeatAt(CurrentTime) - GetBeatAt(speedTime)) / duration, 0.0, 1.0);
			}

			auto lerpedMultiplier = speedProgress * (currentValue - previousValue) + previousValue;

			return lerpedMultiplier;
		}

		double PlayerChartData::GetTimeAtBeat(double beat, double drift) const
		{
			return TimeAtBeat(ConnectedDifficulty->Timing, ConnectedDifficulty->Offset + drift, beat) + StopTimeAtBeat(ConnectedDifficulty->Data->Stops, beat);
		}

		double PlayerChartData::GetOffset() const
		{
			return ConnectedDifficulty->Offset;
		}

		std::map<int, std::string> PlayerChartData::GetSoundList() const
		{
			assert(ConnectedDifficulty && ConnectedDifficulty->Data);
			return ConnectedDifficulty->Data->SoundList;
		}

		ChartType PlayerChartData::GetChartType() const
		{
			assert(ConnectedDifficulty && ConnectedDifficulty->Data);
			assert(ConnectedDifficulty->Data->TimingInfo);
			return ConnectedDifficulty->Data->TimingInfo->GetType();
		}

		bool PlayerChartData::IsBmson() const
		{
			return GetChartType() == TI_BMS &&
				dynamic_cast<BMSChartInfo*>(ConnectedDifficulty->Data->TimingInfo.get())->IsBMSON;
		}

		bool PlayerChartData::IsVirtual() const
		{
			return ConnectedDifficulty->IsVirtual;
		}

		bool PlayerChartData::HasTimingData() const
		{
			assert(ConnectedDifficulty != nullptr);
			return ConnectedDifficulty->Timing.size() > 0;
		}

		SliceContainer PlayerChartData::GetSliceData() const
		{
			return ConnectedDifficulty->Data->SliceData;
		}

		double PlayerChartData::GetDisplacementAt(double Time) const
		{
			return IntegrateToTime(VSpeeds, Time);
		}

		double PlayerChartData::GetDyAt(double Time) const
		{
			return SectionValue(VSpeeds, GetWarpedSongTime(Time));
		}

		void PlayerChartData::DisableNotesUntil(double Time)
		{
			ResetNotes();
			for (auto k = 0U; k < MAX_CHANNELS; k++)
				for (auto m = NotesByChannel[k].begin(); m != NotesByChannel[k].end(); ++m)
					if (m->GetStartTime() <= Time)
						m->Disable();
		}

		void PlayerChartData::ResetNotes()
		{
			for (auto k = 0U; k < MAX_CHANNELS; k++)
				for (auto m = NotesByChannel[k].begin(); m != NotesByChannel[k].end(); ++m)
					m->Reset();
		}
	}
}
