#include <fstream>
#include <sstream>
#include <cassert>
#include <filesystem>
#include "rmath.h"
#include "TextAndFileUtil.h"
#include <game/Song.h>
#include <game/Converter.h>

class BMSConverter : public RowifiedDifficulty {
	rd::Song *Song;

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
	uint32_t GetIndexForValue(std::vector<T> &vec, T value)
	{
		bool found = false;
		uint32_t index = 0;
		for (size_t i = 0; i < vec.size(); i++) {
			if (vec[i] == value) {
				index = i;
				found = true;
				break;
			}
		}

		// Didn't find it. Push it.
		if (!found) {
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
				auto Beat = QuantizeFunction(IntegrateToTime(BPS, T.Time));
				auto BPM = 60 * T.Value;

				// Check redundant BPMs.
				auto index = GetIndexForValue(BPMs, BPM);

				// Create new event at measure.
				auto MeasureForEvent = MeasureForBeat(Beat);
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
			uint32_t index = GetIndexForValue(Scrolls, S.Value);

			if (Beat < 0) continue;

			uint32_t MeasureForEvent = MeasureForBeat(Beat);
			ResizeTimingMeasures(MeasureForEvent);
			TimingMeasures[MeasureForEvent].ScrollEvents.push_back({
				FractionForMeasure(MeasureForEvent, Beat),
				index + 1
			});
		}
	}


	static std::string ToBMSBase36(int n);

	void WriteHeader()
	{
		using std::endl;
		//OutFile << "-- " << RAINDROP_WINDOWTITLE << RAINDROP_VERSIONTEXT << " converter to BMS" << endl;
		OutFile << "-- HEADER" << endl;
		OutFile << "#ARTIST " << Song->Artist << endl;
		OutFile << "#TITLE " << Song->Title << endl;
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
		for (auto i : Parent->Data->SoundList) {
			OutFile << "#WAV" << ToBMSBase36(i.first) << " " << i.second << endl;
		}

		OutFile << endl << "-- BPMs" << endl;
		for (size_t i = 0; i < BPMs.size(); i++) {
			OutFile << "#BPM" << ToBMSBase36(i + 1) << " " << BPMs[i] << endl;
		}

		OutFile << endl << "-- STOPs" << endl;
		for (size_t i = 0; i < Stops.size(); i++) {
			OutFile << "#STOP" << ToBMSBase36(i + 1) << " " << Stops[i] << endl;
		}

		OutFile << endl << "-- SCROLLs" << endl;
		for (size_t i = 0; i < Scrolls.size(); i++)
		{
			OutFile << "#SCROLL" << ToBMSBase36(i + 1) << " " << Scrolls[i] << endl;
		}
	}

	void WriteVectorToMeasureChannel(std::vector<Event> &EventList, int Measure, int Channel, bool AllowMultiple = false)
	{
		if (EventList.size() == 0) return; // Nothing to write.

		auto VecLCM = GetRowCount(EventList);
		sort(EventList.begin(), EventList.end(), [](const Event& A, const Event&B)
			-> bool {
			return ((double)A.Sect.Num / A.Sect.Den) < ((double)B.Sect.Num / B.Sect.Den);
		});

		// first of the pair is numerator aka row given veclcm as a denominator
		// second is event at numerator aka row
		std::map<int, std::vector<int>> rowified;

		// simultaneous event count
		size_t linecount = 0;

		// Now that we have the LCM we can easily just place the objects exactly as we want to output them.
		for (auto &Obj : EventList) {

			// We convert to a numerator that fits with the LCM.
			auto newNumerator = Obj.Sect.Num * VecLCM / Obj.Sect.Den;
			auto &it = rowified[newNumerator];

			if (!it.size() || AllowMultiple) {
				it.push_back(Obj.Evt);

				// max amount of simultaneous events are given by the largest vector
				linecount = std::max(linecount, it.size());
			}
		}

		std::vector<std::stringstream> lines(linecount);

		// add the tag to all lines.
		for (size_t i = 0; i < linecount; i++) {
			auto &line = lines[i];
			line << Utility::Format("#%03d%s:", Measure, ToBMSBase36(Channel).c_str());
		}

		if (rowified.find(0) == rowified.end())
			rowified[0].resize(0);

		for (auto row = rowified.begin(); row != rowified.end(); row++) {

			// ith line gets the ith column at fraction row->first / LCM
			for (size_t i = 0; i < linecount; i++) {
				auto &line = lines[i];

				if (i < row->second.size()) {
					line << ToBMSBase36(row->second[i]);
				}
				else
					line << "00";
			}

			size_t next_row;
			auto next = row;
			next++;

			if (next != rowified.end()) {
				next_row = next->first;
			}
			else {
				next_row = VecLCM;
			}

			// don't count the destination row itself (take 1)
			size_t zero_fill = next_row - row->first - 1;
			for (size_t i = 0; i < zero_fill; i++) {
				for (auto &line : lines) {
					line << "00";
				}
			}

		}

		for (auto &line : lines) {
			OutFile << line.str();
			OutFile << std::endl;
		}
	}

	int GetChannel(int channel) const;

	int GetLNChannel(int channel) const;

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

	BMSConverter(bool Quantize, rd::Difficulty *Source, rd::Song *Song)
		: RowifiedDifficulty(Source, Quantize, true)
	{
		this->Song = Song;

		CalculateTimingPoints();
		CalculateStops();
		CalculateScrolls();
	}

	void Output(std::filesystem::path PathOut)
	{
		std::filesystem::path name = PathOut / Utility::Format("%s (%s) - %s.bms", 
			Song->Title.c_str(), Parent->Name.c_str(), Parent->Author.c_str());

		std::ofstream out(name.string());

		if (!out.is_open())
		{
			auto s = Utility::Format("failed to open file %s", Conversion::ToU8(name.wstring()).c_str());
			throw std::runtime_error(s.c_str());
		}
		if (BPS.size() == 0)
			throw std::runtime_error("There are no timing points!");
		WriteBMSOutput();
		out << OutFile.str();        
    }
};

std::string BMSConverter::ToBMSBase36(int num)
{
    char digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int i;
    char buf[66];

    /* if num is zero */
    if (!num)
        return "00";

	if (num > 1295) // ZZ in b36
		throw std::runtime_error("EventList of range number for BMS conversion");

    buf[65] = '\0';
    i = 65;

	// az simplification: only positive values
	while (num) { /* until num is 0... */
		/* go left 1 digit, divide by radix, and set digit to remainder */
		buf[--i] = digits[num % 36];
		num /= 36;
	}

	if (i == 64) // 64 only one digit?
		buf[i - 1] = '0';

    return &buf[63]; // 63, 64, 65 (two digits + null terminator indices)
}

int BMSConverter::GetChannel(int channel) const
{
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

int BMSConverter::GetLNChannel(int channel) const
{
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

void ConvertBMSAll(rd::Song *Source, std::filesystem::path PathOut, bool Quantize)
{
    for (auto &Diff : Source->Difficulties)
    {
        BMSConverter Conv(Quantize, Diff.get(), Source);
        Conv.Output(PathOut);
    }
}

void ExportToBMS(rd::Song* Source, std::filesystem::path PathOut)
{
    ConvertBMSAll(Source, PathOut, true);
}

void ExportToBMSUnquantized(rd::Song* Source, std::filesystem::path PathOut)
{
    ConvertBMSAll(Source, PathOut, false);
}
