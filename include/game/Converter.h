#pragma once

#include <functional>

class RowifiedDifficulty
{
public:
    struct Event
    {
        IFraction Sect;
        uint32_t Evt;
    };

    struct Measure
    {
        std::vector<Event> Objects[rd::MAX_CHANNELS];
        std::vector<Event> LNObjects[rd::MAX_CHANNELS];
        std::vector<Event> BGMEvents;
    };

private:
    bool Quantizing;
    std::vector<double> MeasureAccomulation;

protected:
    std::function <double(double)> QuantizeFunction;
    static int GetRowCount(const std::vector<Event> &In);

    void CalculateMeasureAccomulation();
    IFraction FractionForMeasure(int Measure, double Beat);

    int MeasureForBeat(double Beat);
    void ResizeMeasures(size_t NewMaxIndex);

    void CalculateBGMEvents();
    void CalculateObjects();

    std::vector<Measure> Measures;
    TimingData BPS;

    rd::Difficulty *Parent{};

    RowifiedDifficulty(rd::Difficulty *Source, bool Quantize, bool CalculateAll);

    friend class rd::Song;
public:
    double GetStartingBPM();
    bool IsQuantized() const;
};

void ConvertToOM(rd::Song *Sng, std::filesystem::path PathOut, std::string Author);
void ConvertToBMS(rd::Song *Sng, std::filesystem::path PathOut);
void ConvertToSMTiming(rd::Song *Sng, std::filesystem::path PathOut);
void ConvertToNPSGraph(rd::Song *Sng, std::filesystem::path PathOut);