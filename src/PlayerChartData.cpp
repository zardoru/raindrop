#include "pch.h"
#include "Song7K.h"
#include "PlayerChartData.h"
#include "Logging.h"

CfgVar DebugMeasurePosGen("MeasurePosGen", "Debug");

namespace Game {
	namespace VSRG {
		PlayerChartState::PlayerChartState()
		{
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
			double Offset)
		{
			if (TimingType == VSRG::Difficulty::BT_BEAT) // Time is in Beats
			{
				return TimeAtBeat(Timing, Offset, Time) + StopTimeAtBeat(StopsTiming, Time);
			}
			else if (TimingType == VSRG::Difficulty::BT_MS || TimingType == VSRG::Difficulty::BT_BEATSPACE) // Time is in MS
			{
				return Time + Offset;
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

		TimingData GetBPSData(VSRG::Difficulty *difficulty)
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

				Seg.Time = TimeFromTimingKind(Timing, StopsTiming, Time.Time, BPMType, Offset);
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
				double StopStartTime = TimeAtBeat(Timing, Offset, Time->Time) + StopTimeAtBeat(StopsTiming, Time->Time);
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

		TimingData GetVSpeeds(TimingData& BPS, double ConstantUserSpeed)
		{
			TimingData VerticalSpeeds;

			// We're using a CMod, so further processing is pointless
			if (ConstantUserSpeed)
			{
				VerticalSpeeds.push_back(TimingSegment(0, ConstantUserSpeed));
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
					VerticalSpeed = UNITS_PER_MEASURE / (spb * 4);
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

		TimingData ApplySpeedChanges(TimingData VerticalSpeeds, TimingData Scrolls, double Offset, bool Reset)
		{
			std::sort(Scrolls.begin(), Scrolls.end());

			const auto Unmodified = VerticalSpeeds;

			for (TimingData::const_iterator Change = Scrolls.begin();
			Change != Scrolls.end();
				++Change)
			{
				TimingData::const_iterator NextChange = (Change + 1);
				double ChangeTime = Change->Time + Offset;

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

		double PlayerChartState::GetWarpAmount(double Time) const
		{
			double wAmt = 0;
			for (auto warp : Warps)
			{
				if (warp.Time < Time)
					wAmt += warp.Value;
			}

			return wAmt;
		}

		bool PlayerChartState::IsWarpingAt(double start_time) const
		{
			auto it = std::lower_bound(Warps.begin(), Warps.end(), start_time, TimeSegmentCompare<TimingSegment>);
			if (it != Warps.end())
				return it->Time + it->Value > start_time;
			else
				return false;
		}


		PlayerChartState PlayerChartState::FromDifficulty(Difficulty *diff, double ConstantUserSpeed)
		{
			PlayerChartState out;
			auto &data = diff->Data;
			if (data == nullptr)
				throw std::runtime_error("Tried to pass a metadata-only difficulty to Player Chart data generator.\n");

			out.ConnectedDifficulty = diff;
			out.BPS = GetBPSData(diff);
			out.ScrollSpeeds = GetVSpeeds(out.BPS, ConstantUserSpeed);


			out.HasTurntable = diff->Data->Turntable;


			if (!ConstantUserSpeed) {
				out.ScrollSpeeds = ApplySpeedChanges(out.ScrollSpeeds, data->Scrolls, diff->Offset,
					diff->BPMType == VSRG::Difficulty::BT_BEATSPACE);

				out.Warps = data->Warps;
				out.InterpoloatedSpeedMultipliers = data->InterpoloatedSpeedMultipliers;
			}

			// this has to go _after_ speed changes were applied
			out.MeasureBarlines = out.GetMeasureLines();

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

						auto VerticalPosition = IntegrateToTime(out.ScrollSpeeds, NewNote.GetStartTime());
						auto HoldEndPosition = IntegrateToTime(out.ScrollSpeeds, NewNote.GetEndTime());

						// if upscroll change minus for plus as well as matrix at screengameplay7k
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
						if (!ConstantUserSpeed || (NewNote.IsJudgable() && !out.IsWarpingAt(CurrentNote.StartTime)))
							out.Notes[KeyIndex].push_back(NewNote);
					}

					MIdx++;
					MsrBeat += Msr.Length;
				}
			}

			// Toggle whether we can use our guarantees for optimizations or not at rendering/judge time.
			out.HasNegativeScroll = false;

			for (auto S : out.InterpoloatedSpeedMultipliers) if (S.Value < 0) out.HasNegativeScroll = true;
			for (auto S : out.ScrollSpeeds) if (S.Value < 0) out.HasNegativeScroll = true;
			return out;
		}

		void PlayerChartState::PrepareOrderedNotes()
		{
			auto diff = ConnectedDifficulty;


			// We do this post push_back 
			// since pointers are invalidated at every turn back there.
			for (int i = 0; i < diff->Channels; i++) {
				NotesVerticallyOrdered[i].clear();
				NotesTimeOrdered[i].clear();

				for (int j = 0; j < Notes[i].size(); j++) {
					auto ptr = &(Notes[i][j]);
					NotesVerticallyOrdered[i].push_back(ptr);
					NotesTimeOrdered[i].push_back(ptr);
				}

				NotesVerticallyOrdered[i].shrink_to_fit();
				NotesTimeOrdered[i].shrink_to_fit();
			}

			for (int i = 0; i < diff->Channels; i++) {

				// done with the channel - sort it
				std::stable_sort(
					NotesVerticallyOrdered[i].begin(),
					NotesVerticallyOrdered[i].end(),
					[](const TrackNote* A, const TrackNote* B) -> bool
				{
					return A->GetVertical() < B->GetVertical();
				});

				std::stable_sort(
					NotesTimeOrdered[i].begin(),
					NotesTimeOrdered[i].end(),
					[](const TrackNote *A, const TrackNote *B) -> bool
				{
					return A->GetStartTime() < B->GetStartTime();
				});
			}
		}

		// audio time -> chart time
		double PlayerChartState::GetWarpedSongTime(double SongTime) const
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


		std::vector<double> PlayerChartState::GetMeasureLines() const
		{
			auto &diff = ConnectedDifficulty;
			auto &Data = diff->Data;
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
			double PreTime = WaitTime + diff->Offset;
			double PreTimeBeats = BPS * PreTime;
			int TotMeasures = PreTimeBeats / Data->Measures[0].Length;
			double MeasureTime = 1 / BPS * Data->Measures[0].Length;

			if (DebugMeasurePosGen)
				Log::LogPrintf("Total pre-offset measures: %d (pt: %f, ptb: %f, mtime: %f)\n", TotMeasures, PreTime, PreTimeBeats, MeasureTime);

			for (auto i = 0; i < TotMeasures; i++)
			{
				auto T = diff->Offset - MeasureTime * i;
				auto PositionOut = IntegrateToTime(ScrollSpeeds, T);
				Out.push_back(PositionOut);
				if (DebugMeasurePosGen)
					Log::LogPrintf("Add measure line at time %f (Vertical %f)\n", T, PositionOut);
			}

			// Add
			for (auto Msr : Data->Measures)
			{
				double PositionOut = 0.0;

				if (BPMType == Difficulty::BT_BEAT)
				{
					PositionOut = IntegrateToTime(ScrollSpeeds, GetTimeAtBeat(Last));
				}
				else if (BPMType == Difficulty::BT_BEATSPACE)
				{
					auto TargetTime = IntegrateToTime(SPB, Last) + diff->Offset;

					PositionOut = IntegrateToTime(ScrollSpeeds, TargetTime);
					if (DebugMeasurePosGen) {
						Log::LogPrintf("Add measure line at time %f (Vertical: %f)\n", TargetTime, PositionOut);
					}
				}

				Out.push_back(PositionOut);
				Last += Msr.Length;
			}

			return Out;
		}

		double PlayerChartState::GetBpmAt(double Time) const
		{
			return SectionValue(BPS, Time) * 60;
		}

		double PlayerChartState::GetBpsAt(double Time) const
		{
			return SectionValue(BPS, Time);
		}


		double PlayerChartState::GetBeatAt(double Time) const
		{
			return IntegrateToTime(BPS, Time);
		}

		double PlayerChartState::GetSpeedMultiplierAt(double Time) const
		{
			// Calculate current speed value to apply.
			auto CurrentTime = GetWarpedSongTime(Time);

			// speedIter: first which time is greater than current
			auto speedIter = lower_bound(InterpoloatedSpeedMultipliers.begin(), InterpoloatedSpeedMultipliers.end(), CurrentTime);
			double previousValue = 1;
			double currentValue = 1;
			double speedTime = CurrentTime;
			double duration = 1;
			bool integrateByBeats = false;

			if (speedIter != InterpoloatedSpeedMultipliers.begin())
				speedIter--;

			// Do we have a speed that has a value to interpolate from?
			/*
				Elaboration:
				From the speed at the end of the previous speed, interpolate to speedIter->value
				in speedIter->duration time across the current time - speedIter->time.
			*/
			if (speedIter != InterpoloatedSpeedMultipliers.begin())
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
				if (speedIter != InterpoloatedSpeedMultipliers.end())
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

		// returns chart time
		double PlayerChartState::GetTimeAtBeat(double beat) const
		{
			return TimeAtBeat(ConnectedDifficulty->Timing, ConnectedDifficulty->Offset, beat) + 
				StopTimeAtBeat(ConnectedDifficulty->Data->Stops, beat);
		}

		double PlayerChartState::GetOffset() const
		{
			return ConnectedDifficulty->Offset;
		}

		double PlayerChartState::GetMeasureTime(double msr) const
		{
			double beat = 0;

			if (msr <= 0)
			{
				return 0;
			}

			auto whole = int(floor(msr));
			auto fraction = msr - double(whole);
			for (auto i = 0; i < whole; i++)
				beat += ConnectedDifficulty->Data->Measures[i].Length;
			beat += ConnectedDifficulty->Data->Measures[whole].Length * fraction;

			// Log::Logf("Warping to measure measure %d at beat %f.\n", whole, beat);

			return GetTimeAtBeat(beat);
		}


		std::map<int, std::string> PlayerChartState::GetSoundList() const
		{
			assert(ConnectedDifficulty && ConnectedDifficulty->Data);
			return ConnectedDifficulty->Data->SoundList;
		}

		ChartType PlayerChartState::GetChartType() const
		{
			assert(ConnectedDifficulty && ConnectedDifficulty->Data);
			assert(ConnectedDifficulty->Data->TimingInfo);
			return ConnectedDifficulty->Data->TimingInfo->GetType();
		}

		bool PlayerChartState::IsBmson() const
		{
			return GetChartType() == TI_BMS &&
				dynamic_cast<BMSChartInfo*>(ConnectedDifficulty->Data->TimingInfo.get())->IsBMSON;
		}

		bool PlayerChartState::IsVirtual() const
		{
			return ConnectedDifficulty->IsVirtual;
		}

		bool PlayerChartState::HasTimingData() const
		{
			assert(ConnectedDifficulty != nullptr);
			return ConnectedDifficulty->Timing.size() > 0;
		}

		SliceContainer PlayerChartState::GetSliceData() const
		{
			return ConnectedDifficulty->Data->SliceData;
		}

		// chart time -> note displacement
		double PlayerChartState::GetChartDisplacementAt(double Time) const
		{
			return IntegrateToTime(ScrollSpeeds, Time);
		}

		// song time -> scroll speed
		double PlayerChartState::GetDisplacementSpeedAt(double Time) const
		{
			return SectionValue(ScrollSpeeds, GetWarpedSongTime(Time));
		}

		void PlayerChartState::DisableNotesUntil(double Time)
		{
			ResetNotes();
			for (auto k = 0U; k < MAX_CHANNELS; k++)
				for (auto m = Notes[k].begin(); m != Notes[k].end(); ++m)
					if (m->GetStartTime() <= Time)
						m->Disable();
		}

		void PlayerChartState::ResetNotes()
		{
			for (auto k = 0U; k < MAX_CHANNELS; k++)
				for (auto m = Notes[k].begin(); m != Notes[k].end(); ++m)
					m->Reset();
		}
	}
}
