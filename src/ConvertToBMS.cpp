#include <fstream>
#include "GameGlobal.h"
#include "Logging.h"
#include "Song7K.h"

#include <boost/math/common_factor.hpp>
#include <boost/format.hpp>

// lcm of a set of integers
int LCM(vector<int> Set) {
	int current = Set.at(0);

	for (size_t i = 1; i < Set.size(); i++) {
		current = boost::math::lcm(current, Set[i]);
	}
	return current;
}

double PassThrough(double D){
	return D;
}

class BMSConverter {

	function <double(double)> QuantizeFunction;

	template <class T>
	struct Fraction {
		T Num;
		T Den;

		Fraction()
		{
			Num = Den = 1;
		}

		template <class K>
		Fraction(K num, K den) {
			Num = num;
			Den = den;
		}

		void fromDouble(double in)
		{
			double d = 0;
			Num = 0;
			Den = 1;
			while (d != in)
			{
				if (d < in)
					Num++;
				else if (d > in)
					Den++;
				d = (double)Num / Den;
			}
		}
	};
	
	typedef Fraction<long long> LFraction;
	typedef Fraction<int> IFraction;

	struct OutObj{
		IFraction Sect;
		int Evt;
	};

	struct Measure{
		vector<OutObj> Objects[VSRG::MAX_CHANNELS];
		vector<OutObj> LNObjects[VSRG::MAX_CHANNELS];
		vector<OutObj> BPMEvents;
		vector<OutObj> StopEvents;
		vector<OutObj> BGMEvents;

		double bmsLength;

		Measure() {
			bmsLength = 1;
		}

		static int GetRowCount(const vector<OutObj> &In){
			// literally the only hard part of this
			// We have to find the LCM of the set of fractions given by the Fraction of all objects in the vector.
			std::vector <int> Denominators;

			// Find all different denominators.
			for (auto i : In) {
				for (auto k : Denominators)
				{
					if (i.Sect.Den == k)
						goto next_object;
				}

				Denominators.push_back(i.Sect.Den);
			next_object:;
			}

			if (Denominators.size() == 1) return Denominators.at(0);

			// Now get the LCM.
			return LCM(Denominators);
		}
	};

	struct BMSFile {
		vector<Measure> Measures;
		TimingData BPS;
		vector<double> BPMs;
		vector<int> Stops;
	} *CurrentBMS;

	VSRG::Difficulty *CurrentDifficulty;
	VSRG::Song *Song;

	std::vector<double> MeasureAccomulation;
	std::stringstream OutFile;

	void CalculateMeasureAccomulation() {
		double Acom = 0;

		MeasureAccomulation.clear();
		for (auto M : CurrentDifficulty->Data->Measures){
			MeasureAccomulation.push_back(QuantizeFunction(Acom));
			Acom += M.MeasureLength;
		}
	}

	IFraction FractionForMeasure(int Measure, double Beat)
	{
		double mStart = MeasureAccomulation[Measure];
		double mLen = CurrentDifficulty->Data->Measures[Measure].MeasureLength;
		LFraction Frac;
		Frac.fromDouble(QuantizeFractionMeasure((Beat - mStart) / mLen));
		return IFraction{ Frac.Num, Frac.Den };
	}

	int MeasureForBeat(double Beat)
	{
		int Measure = -1;
		for (auto i : MeasureAccomulation)
		{
			if (Beat >= i)
			{
				Measure += 1;
			}
			else return Measure;
		}

		assert(0);
	}

	void ResizeMeasures(size_t NewMaxIndex) {
		if (CurrentBMS->Measures.size() < NewMaxIndex+1)
			CurrentBMS->Measures.resize(NewMaxIndex + 1);
	}

	void CalculateTimingPoints()
	{
		/*
		 Group all different BPM changes that are different.
		 Use that index as the event value. When outputting, check if result can be represented as an integer in base 16.
		 If it can, output as BPM, otherwise, as EXBPM.

		 Assumptions: There are no BPS events before time 0.
		*/
		for (auto T : CurrentBMS->BPS)
		{
			assert(T.Time >= 0);
			if (T.Value != 0) // Not a stop.
			{
				double Beat = QuantizeFunction(IntegrateToTime(CurrentBMS->BPS, T.Time));
				double BPM = 60 * T.Value;

				int index = -1;

				// Check redundant BPMs.
				for (size_t i = 0; i < CurrentBMS->BPMs.size(); i++) {
					if (CurrentBMS->BPMs[i] == BPM) {
						index = i;
						break;
					}
				}

				// Didn't find it. Push it.
				if (index == -1) {
					CurrentBMS->BPMs.push_back(BPM);
					index = CurrentBMS->BPMs.size() - 1;
				}

				// Create new event at measure.
				int MeasureForEvent = MeasureForBeat(Beat);
				ResizeMeasures(MeasureForEvent); // Make sure we've got space on the measures vector
				CurrentBMS->Measures[MeasureForEvent].BPMEvents.push_back({ FractionForMeasure(MeasureForEvent, Beat), index+1 });
			}
		}

		int MeasureNum = 0;
		for (auto M : CurrentDifficulty->Data->Measures) {
			double bmsMult = M.MeasureLength / 4; // 4 * x = mlen, x = mlen / 4

			ResizeMeasures(MeasureNum);
			CurrentBMS->Measures[MeasureNum].bmsLength = bmsMult;

			MeasureNum++;
		}
	}

	void CalculateStops()
	{
		for (auto T = CurrentBMS->BPS.begin(); T != CurrentBMS->BPS.end(); ++T)
		{
			assert(T->Time >= 0);
			if (T->Value == 0) // A stop.
			{
				auto RestBPM = T + 1;
				double Beat = QuantizeFunction(IntegrateToTime(CurrentBMS->BPS, T->Time));
				// By song processing law, any stop is followed by a restoration of the original BPM.
				double Duration = RestBPM->Time - T->Time;
				double BPSatStop = RestBPM->Value; 
				// We need to know how long in beats this stop lasts for BPSatStop.
				double StopBMSDurationBeats = BPSatStop * Duration;
				// Now the duration in BMS stops..
				int StopDurationBMS = round(StopBMSDurationBeats * 48.0);

				int index = -1;

				// Check redundant stops.
				for (size_t i = 0; i < CurrentBMS->Stops.size(); i++) {
					if (CurrentBMS->Stops[i] == StopDurationBMS) {
						index = i;
						break;
					}
				}

				// Didn't find it. Push it.
				if (index == -1) {
					CurrentBMS->Stops.push_back(StopDurationBMS);
					index = CurrentBMS->BPMs.size() - 1;
				}

				int MeasureForEvent = MeasureForBeat(Beat);
				ResizeMeasures(MeasureForEvent);
				CurrentBMS->Measures[MeasureForEvent].StopEvents.push_back({ FractionForMeasure(MeasureForEvent, Beat), index+1 });
			}
		}
	}

	void CalculateBGMEvents()
	{
		for (auto BGM : CurrentDifficulty->Data->BGMEvents) {
			double Beat = QuantizeFunction(IntegrateToTime(CurrentBMS->BPS, BGM.Time));

			int MeasureForEvent = MeasureForBeat(Beat);
			ResizeMeasures(MeasureForEvent);
			CurrentBMS->Measures[MeasureForEvent].BGMEvents.push_back({ FractionForMeasure(MeasureForEvent, Beat), BGM.Sound });
		}
	}

	void CalculateObjects()
	{
		for (auto M : CurrentDifficulty->Data->Measures) {
			for (int K = 0; K < CurrentDifficulty->Channels; K++) {
				for (auto N : M.MeasureNotes[K]) {
					double StartBeat = QuantizeFunction(IntegrateToTime(CurrentBMS->BPS, N.StartTime));
					if (N.EndTime == 0){ // Non-hold. Emit channels 11-...

						int MeasureForEvent = MeasureForBeat(StartBeat);
						ResizeMeasures(MeasureForEvent);

						CurrentBMS->Measures[MeasureForEvent].Objects[K].push_back({ FractionForMeasure(MeasureForEvent, StartBeat), N.Sound });
					} else { // Hold. Emit channels 51-...
						double EndBeat = QuantizeFunction(IntegrateToTime(CurrentBMS->BPS, N.EndTime));
						int MeasureForEvent = MeasureForBeat(StartBeat);
						int MeasureForEventEnd = MeasureForBeat(EndBeat);
						ResizeMeasures(MeasureForEvent);

						CurrentBMS->Measures[MeasureForEvent].LNObjects[K].push_back({ FractionForMeasure(MeasureForEvent, StartBeat), N.Sound });
						CurrentBMS->Measures[MeasureForEventEnd].LNObjects[K].push_back({ FractionForMeasure(MeasureForEventEnd, EndBeat), N.Sound });
					}
				}
			}
		}
	}

	double GetStartingBPM() {
		return CurrentBMS->BPS[0].Value * 60;
	}

	GString ToBase36(int n) {
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
		OutFile << "-- " << RAINDROP_VERSIONTEXT << " converter to BMS" << endl;
		OutFile << "-- HEADER" << endl;
		OutFile << "#ARTIST " << Song->SongAuthor << endl;
		OutFile << "#TITLE " << Song->SongName << endl;
		OutFile << "#MUSIC " << Song->SongFilename << endl;
		OutFile << "#OFFSET " << CurrentDifficulty->Offset << endl;
		OutFile << "#BPM " << GetStartingBPM() << endl;
		OutFile << "#PREVIEWTIME " << Song->PreviewTime << endl;
		OutFile << "#STAGEFILE " << CurrentDifficulty->Data->StageFile << endl;
		OutFile << "#DIFFICULTY " << CurrentDifficulty->Name << endl;
		OutFile << "#PREVIEW " << Song->SongPreviewSource << endl;
		OutFile << "#PLAYLEVEL " << CurrentDifficulty->Level << endl;
		OutFile << "#MAKER " << CurrentDifficulty->Author << endl;

		OutFile << endl << "-- WAVs" << endl;
		for (auto i : CurrentDifficulty->SoundList){
			OutFile << "#WAV" << ToBase36(i.first) << " " << i.second << endl;
		}

		OutFile << endl << "-- BPMs" << endl;
		for (size_t i = 0; i < CurrentBMS->BPMs.size(); i++){
			OutFile << "#EXBPM" << ToBase36(i + 1) << " " << CurrentBMS->BPMs[i] << endl;
		}

		OutFile << endl << "-- STOPs" << endl;
		for (size_t i = 0; i < CurrentBMS->Stops.size(); i++){
			OutFile << "#STOP" << ToBase36(i + 1) << " " << CurrentBMS->Stops[i] << endl;
		}
	}

	void WriteVectorToMeasureChannel(vector<OutObj> &Out, int Measure, int Channel) 
	{
		if (Out.size() == 0) return; // Nothing to write.

		int VecLCM = Measure::GetRowCount(Out);
		std::sort(Out.begin(), Out.end(), [](const OutObj& A, const OutObj&B)
			-> bool {
			double dA = (double)A.Sect.Num / A.Sect.Den;
			double dB = (double)B.Sect.Num / B.Sect.Den;
			return dA < dB;
		});

		vector<int> rowified; 
		rowified.resize(VecLCM);

		// Now that we have LCM units we can easily just place the objects exactly as we want to output them.
		for (auto Obj : Out){ // We convert to a fraction that fits with the LCM.
			int rNum = Obj.Sect.Num * VecLCM / Obj.Sect.Den;
			rowified[rNum] = Obj.Evt;
		}

		OutFile << boost::format("#%03d%s:") % Measure % ToBase36(Channel);
		for (auto val : rowified){
			OutFile << ToBase36(val);
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
		int Measure = 0;
		using std::endl;
		for (auto M : CurrentBMS->Measures){
			if (M.bmsLength != 1)
				OutFile << boost::format("#%03d02:%f") % Measure % M.bmsLength << endl;

			OutFile << "-- BGM - Measure " << Measure << endl;
			WriteVectorToMeasureChannel(M.BGMEvents, Measure, 1);

			OutFile << "-- BPM" << endl;
			WriteVectorToMeasureChannel(M.BPMEvents, Measure, 8); // lol just exbpm. who cares anyway

			OutFile << "-- OBJ" << endl;
			for (int i = 0; i < CurrentDifficulty->Channels; i++)
			{
				WriteVectorToMeasureChannel(M.Objects[i], Measure, GetChannel(i));
				WriteVectorToMeasureChannel(M.LNObjects[i], Measure, GetLNChannel(i));
			}

			OutFile << "-- STOPS" << endl;
			WriteVectorToMeasureChannel(M.StopEvents, Measure, 9);
			Measure++;
		}
	}

	void WriteBMSOutput()
	{
		WriteHeader();
		WriteMeasures();
	}

public:

	BMSConverter(bool Quantize)
	{
		if (Quantize)
			QuantizeFunction = bind(QuantizeBeat, _1);
		else
			QuantizeFunction = bind(PassThrough, _1);
	}

	void Convert(VSRG::Song* Source, Directory PathOut)
	{
		if (!Source) throw std::runtime_error("Source file unreadable.");

		Song = Source;

		for (auto Difficulty : Source->Difficulties) {
			BMSFile bms;
			GString name = (boost::format("%1%/%2%-%3%.bms") % PathOut.c_path() % Difficulty->Name % Difficulty->Author).str();
			std::ofstream out(name.c_str());

			if (!out.is_open())
				Log::Printf("failed to open file %s", Utility::Widen(name).c_str());
			
			CurrentBMS = &bms;
			CurrentDifficulty = Difficulty.get();

			Difficulty->Process(NULL, CurrentBMS->BPS, TimingData()); // We need the BPS from here.
			CalculateMeasureAccomulation();
			CalculateTimingPoints();
			CalculateStops();
			CalculateBGMEvents();
			CalculateObjects();

			WriteBMSOutput();
			out << OutFile.str();
			OutFile = std::stringstream();
		}
	}
};


void ExportToBMS(VSRG::Song* Source, Directory PathOut)
{
	BMSConverter Conv(true);
	Conv.Convert(Source, PathOut);
}

void ExportToBMSUnquantized(VSRG::Song* Source, Directory PathOut)
{
	BMSConverter Conv(false);
	Conv.Convert(Source, PathOut);
}