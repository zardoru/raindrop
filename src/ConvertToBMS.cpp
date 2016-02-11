#include "pch.h"

#include "GameGlobal.h"
#include "Logging.h"
#include "Song7K.h"

class BMSConverter : public VSRG::RowifiedDifficulty {
	VSRG::Song *Song;

	std::stringstream OutFile;

	struct TimingMeasure {
		std::vector<Event> BPMEvents;
		std::vector<Event> StopEvents;
		std::vector<Event> ScrollEvents;
	};

	std::vector<TimingMeasure> TimingMeasures;
	std::vector<double> BPMs;
	std::vector<int> Stops;
	std::vector<double> Scrolls;

	void ResizeTimingMeasures(size_t NewMaxIndex) {
		if (TimingMeasures.size() < NewMaxIndex + 1) {
			TimingMeasures.resize(NewMaxIndex + 1);
		}
	}

	template <class T>
	int GetIndexForValue(std::vector<T> &vec, T value)
	{
		auto index = -1;
		for (size_t i = 0; i < vec.size(); i++) {
			if (vec[i] == value) {
				index = i;
				break;
			}
		}

		// Didn't find it. Push it.
		if (index == -1) {
			vec.push_back(value);
			index = vec.size() - 1;
		}

		return index;
	}

	void CalculateTimingPoints()
	{
		/*
		Group all different BPM changes that are different.
		Use that index as the event value. When outputting, check if result can be represented as an integer in base 16.
		If it can, output as BPM, otherwise, as EXBPM.

		Or don't. Whatever.

		Assumptions: There are no BPS events before time 0.
		*/
		for (auto T : BPS)
		{
			assert(T.Time >= 0);
			if (T.Value != 0) // Not a stop.
			{
				double Beat = QuantizeFunction(IntegrateToTime(BPS, T.Time));
				double BPM = 60 * T.Value;

				// Check redundant BPMs.
				int index = GetIndexForValue(BPMs, BPM);

				// Create new event at measure.
				int MeasureForEvent = MeasureForBeat(Beat);
				ResizeTimingMeasures(MeasureForEvent); // Make sure we've got space on the measures std::vector
				TimingMeasures[MeasureForEvent].BPMEvents.push_back({ 
					FractionForMeasure(MeasureForEvent, Beat),
					index + 1 
				});
			}
		}
	}

	void CalculateStops()
	{
		for (auto T = BPS.begin(); T != BPS.end(); ++T)
		{
			assert(T->Time >= 0);
			if (T->Value <= 0.0001) // A stop.
			{
				auto RestBPM = T + 1;
				auto Beat = QuantizeFunction(IntegrateToTime(BPS, T->Time));
				// By song processing law, any stop is followed by a restoration of the original BPM.
				auto Duration = RestBPM->Time - T->Time;
				auto BPSatStop = RestBPM->Value;
				// We need to know how long in beats this stop lasts for BPSatStop.
				auto StopBMSDurationBeats = BPSatStop * Duration;
				// Now the duration in BMS stops..
				int StopDurationBMS = round(StopBMSDurationBeats * 48.0);

				// Check redundant stops.
				auto index = GetIndexForValue(Stops, StopDurationBMS);

				int MeasureForEvent = MeasureForBeat(Beat);
				ResizeTimingMeasures(MeasureForEvent);
				TimingMeasures[MeasureForEvent].StopEvents.push_back({ 
					FractionForMeasure(MeasureForEvent, Beat), 
					index + 1 
				});
			}
		}
	}

	void CalculateScrolls()
	{
		for (auto S : Parent->Data->Scrolls)
		{
			auto Beat = QuantizeFunction(IntegrateToTime(BPS, S.Time));
			auto index = GetIndexForValue(Scrolls, S.Value);

			if (Beat < 0) continue;

			int MeasureForEvent = MeasureForBeat(Beat);
			ResizeTimingMeasures(MeasureForEvent);
			TimingMeasures[MeasureForEvent].ScrollEvents.push_back({
				FractionForMeasure(MeasureForEvent, Beat),
				index + 1
			});
		}
	}


	std::string ToBMSBase36(int n) {
		char pt[32] = {0};
		itoa(n, pt, 36);
		pt[2] = 0;
		if (pt[1] == 0) { // Make it at least, and at most, two digits (for BMS)
			pt[1] = pt[0];
			pt[0] = '0';
		}

		if (pt[0] == 0) {
			pt[0] = '0';
			pt[1] = '0';
		}

		return pt;
	}

	void WriteHeader()
	{
		using std::endl;
		OutFile << "-- " << RAINDROP_WINDOWTITLE << RAINDROP_VERSIONTEXT << " converter to BMS" << endl;
		OutFile << "-- HEADER" << endl;
		OutFile << "#ARTIST " << Song->SongAuthor << endl;
		OutFile << "#TITLE " << Song->SongName << endl;
		OutFile << "#MUSIC " << Song->SongFilename << endl;
		OutFile << "#OFFSET " << Parent->Offset << endl;
		OutFile << "#BPM " << GetStartingBPM() << endl;
		OutFile << "#PREVIEWPOINT " << Song->PreviewTime << endl;
		OutFile << "#STAGEFILE " << Parent->Data->StageFile << endl;
		OutFile << "#DIFFICULTY " << Parent->Name << endl;
		OutFile << "#PREVIEW " << Song->SongPreviewSource << endl;
		OutFile << "#PLAYLEVEL " << Parent->Level << endl;
		OutFile << "#MAKER " << Parent->Author << endl;

		OutFile << endl << "-- WAVs" << endl;
		for (auto i : Parent->SoundList){
			OutFile << "#WAV" << ToBMSBase36(i.first) << " " << i.second << endl;
		}

		OutFile << endl << "-- BPMs" << endl;
		for (size_t i = 0; i < BPMs.size(); i++){
			OutFile << "#BPM" << ToBMSBase36(i + 1) << " " << BPMs[i] << endl;
		}

		OutFile << endl << "-- STOPs" << endl;
		for (size_t i = 0; i < Stops.size(); i++){
			OutFile << "#STOP" << ToBMSBase36(i + 1) << " " << Stops[i] << endl;
		}

		OutFile << endl << "-- SCROLLs" << endl;
		for (size_t i = 0; i < Scrolls.size(); i++)
		{
			OutFile << "#SCROLL" << ToBMSBase36(i + 1) << " " << Scrolls[i] << endl;
		}
	}

	void WriteVectorToMeasureChannel(std::vector<Event> &Out, int Measure, int Channel, bool AllowMultiple = false) 
	{
		if (Out.size() == 0) return; // Nothing to write.

		auto VecLCM = GetRowCount(Out);
		sort(Out.begin(), Out.end(), [](const Event& A, const Event&B)
			-> bool {
			     auto dA = double(A.Sect.Num) / A.Sect.Den;
			     auto dB = double(B.Sect.Num) / B.Sect.Den;
			return dA < dB;
		});

		std::vector<std::vector<int>> rowified; 

		// Now that we have the LCM we can easily just place the objects exactly as we want to output them.
		for (auto Obj : Out) { // We convert to a fraction that fits with the LCM.
			auto rNum = Obj.Sect.Num * VecLCM / Obj.Sect.Den;
			bool slotFree = false;
			for (size_t i = 0; i < rowified.size(); i++) {
				if (!rowified[i][rNum] || !AllowMultiple) {
					rowified[i][rNum] = Obj.Evt;
					slotFree = true;
					break;
				}
			}

			if (!slotFree) // didn't find an unused slot
			{
				std::vector<int> row(VecLCM, 0);
				row[rNum] = Obj.Evt;
				rowified.push_back(row);
			}
		}

		for (auto val : rowified) {
			OutFile << Utility::Format("#%03d%s:", Measure, ToBMSBase36(Channel).c_str());
			for (auto r : val) {
				OutFile << ToBMSBase36(r);
			}
			OutFile << std::endl;
		}
		OutFile << std::endl;
	}

	int GetChannel(int channel) {
		switch (channel) {
		case 0:
			return 42; // scratch
		case 1:
			return 37;
		case 2:
			return 38;
		case 3:
			return 39;
		case 4:
			return 40;
		case 5:
			return 41;
		case 6:
			return 44;
		case 7:
			return 45;
		default:
			return GetChannel(channel - 8) - 37 + 73; // move to 2nd side...
		}
	}

	int GetLNChannel(int channel) {
		switch (channel) {
		case 0:
			return 186; // scratch
		case 1:
			return 181;
		case 2:
			return 182;
		case 3:
			return 183;
		case 4:
			return 184;
		case 5:
			return 185;
		case 6:
			return 188;
		case 7:
			return 189;
		default:
			return GetLNChannel(channel - 8) - 181 + 217; // move to 2nd side...
		}
	}

	void WriteMeasures()
	{
		uint32_t Measure = 0;
		using std::endl;
		for (auto M : Measures){
			if (Parent->Data->Measures[Measure].Length != 4)
			{
				double bmsLength = Parent->Data->Measures[Measure].Length / 4;
				OutFile << Utility::Format("#%03d02:%f", Measure, bmsLength) << endl;
			}

			OutFile << "-- BGM - Measure " << Measure << endl;
			WriteVectorToMeasureChannel(M.BGMEvents, Measure, 1, true);

			if (Measure < TimingMeasures.size()) {

				if (TimingMeasures[Measure].BPMEvents.size()) {
					OutFile << "-- BPM" << endl;
					WriteVectorToMeasureChannel(TimingMeasures[Measure].BPMEvents, Measure, 8); // lol just exbpm. who cares anyway
				}

				if (TimingMeasures[Measure].StopEvents.size()) {
					OutFile << "-- STOPS" << endl;
					WriteVectorToMeasureChannel(TimingMeasures[Measure].StopEvents, Measure, 9);
				}

				if (TimingMeasures[Measure].ScrollEvents.size())
				{
					OutFile << "-- SCROLLS" << endl;
					WriteVectorToMeasureChannel(TimingMeasures[Measure].ScrollEvents, Measure, b36toi("SC"));
				}
			}

			OutFile << "-- OBJ" << endl;
			for (int i = 0; i < Parent->Channels; i++)
			{
				WriteVectorToMeasureChannel(M.Objects[i], Measure, GetChannel(i));
				WriteVectorToMeasureChannel(M.LNObjects[i], Measure, GetLNChannel(i));
			}

			Measure++;
		}
	}

	void WriteBMSOutput()
	{
		WriteHeader();
		WriteMeasures();
	}

public:

	BMSConverter(bool Quantize, VSRG::Difficulty *Source, VSRG::Song *Song) 
		: RowifiedDifficulty(Source, Quantize, true)
	{
		this->Song = Song;

		try {
			CalculateTimingPoints();
			CalculateStops();
			CalculateScrolls();
		} catch (std::exception &e)
		{
			Log::Printf("Error while converting timing data: %s\n", e.what());
		}
	}

	void Output(std::filesystem::path PathOut)
	{
		std::filesystem::path name = PathOut / Utility::Format("%s (%s) - %s.bms", 
			Song->SongName.c_str(), Parent->Name.c_str(), Parent->Author.c_str());

        std::ofstream out(name);

        try
        {
			if (!out.is_open())
			{
				auto s = Utility::Format("failed to open file %s", Utility::Narrow(name).c_str());
				throw std::exception(s.c_str());
			}
            if (BPS.size() == 0)
                throw std::exception("There are no timing points!");
            WriteBMSOutput();
            out << OutFile.str();
        }
        catch (std::exception &e)
        {
            Log::Printf("Error while converting: %s\n", e.what());
        }
    }
};

void ConvertBMSAll(VSRG::Song *Source, std::filesystem::path PathOut, bool Quantize)
{
    for (auto Diff : Source->Difficulties)
    {
        BMSConverter Conv(Quantize, Diff.get(), Source);
        Conv.Output(PathOut);
    }
}

void ExportToBMS(VSRG::Song* Source, std::filesystem::path PathOut)
{
    ConvertBMSAll(Source, PathOut, true);
}

void ExportToBMSUnquantized(VSRG::Song* Source, std::filesystem::path PathOut)
{
    ConvertBMSAll(Source, PathOut, false);
}