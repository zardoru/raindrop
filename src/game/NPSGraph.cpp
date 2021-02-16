#include "rmath.h"
#include "glm.h"

#include <game/GameConstants.h>
#include <game/Song.h>

#include "TextAndFileUtil.h"

#include <fstream>
#include <sstream>

class NPSGraph
{
    rd::Song* Song;

    int CountInterval(rd::Difficulty* In, double timeStart, double timeEnd)
    {
        int out = 0;
        for (auto m : In->Data->Measures)
        {
            for (int k = 0; k < In->Channels; k++)
            {
                for (auto note : m.Notes[k])
                {
                    if (note.StartTime >= timeStart || (note.EndTime >= timeStart && note.EndTime))
                    {
                        if (note.StartTime < timeEnd || (note.EndTime < timeEnd && note.EndTime))
                        {
                            out++;
                        }
                    }
                }
            }
        }

        return out;
    }

public:
    NPSGraph(rd::Song* In)
    {
        Song = In;
    }

    std::vector<int> GetDataPoints(int diffIndex, double intervalduration)
    {
        std::vector<int> datapoints;
        rd::Difficulty *Diff = Song->Difficulties.at(diffIndex).get();
        if (Diff == nullptr) return datapoints;

        int intervalCount = ceil(Diff->Duration / intervalduration);
        for (int i = 0; i < intervalCount; i++)
        {
            datapoints.push_back(CountInterval(Diff, i * intervalduration, (i + 1) * intervalduration));
        }

        return datapoints;
    }

    std::string GetSVGText(int diffIndex, double intervalduration = 1, double peakMargin = 1.2)
    {
		using std::endl;
        std::stringstream out;
        std::vector<int> dataPoints = GetDataPoints(diffIndex, intervalduration);
        auto peak_it = std::max_element(dataPoints.begin(), dataPoints.end());
        float peakf = *peak_it;
        double peak = *peak_it * peakMargin;

        out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">" << endl;

        size_t ptIdx = 0;
        float ImageHeight = 300; //CfgValNPS("GraphHeight", 300);
        float GraphYOffset = 50; //CfgValNPS("GraphYOffs", 50);
        float GraphXOffset = 100; //CfgValNPS("GraphXOffs", 100);
		float TextXOffset = 20; //CfgValNPS("TextXOffs", 20);
		float TextYOffset = 20; //CfgValNPS("TextYOffs", 20);

        float IntervalWidth = 10;
        float GraphWidth = dataPoints.size() * IntervalWidth;

        float RealGraphWidth = 1000; //CfgValNPS("Width", 1000);
        float XRatio = RealGraphWidth / GraphWidth;

        Vec2 BL(GraphXOffset, GraphYOffset + ImageHeight);
        Vec2 BR(RealGraphWidth, 0);
        Vec2 TL(GraphXOffset, GraphYOffset);
        BR += BL;

        std::string DiffAuth = Song->GetDifficulty(diffIndex)->Author;

        if (!DiffAuth.length())
            DiffAuth = "an anonymous charter";

        float avgNPS = Song->GetDifficulty(diffIndex)->Data->GetScoreItemsCount() / Song->GetDifficulty(diffIndex)->Duration;

        out << Utility::Format("<text x=\"%.0f\" y=\"%.0f\" fill=\"black\">%s - %s (%s) by %s (Max NPS: %.2f/Avg NPS: %.2f)</text>",
            TextXOffset, TextYOffset,
            Song->Title.c_str(), Song->Artist.c_str(), 
			Song->GetDifficulty(diffIndex)->Name.c_str(), DiffAuth.c_str(),
            peakf / intervalduration,
            avgNPS) << endl;

        out << Utility::Format("\t<line x1 = \"%.0f\" y1 = \"%.0f\" x2 = \"%.0f\" y2 = \"%.0f\" style = \"stroke:rgb(0,0,0);stroke-width:4\"/>",
            BL.x, BL.y, BR.x, BR.y) << endl;

        out << Utility::Format("\t<line x1 = \"%.0f\" y1 = \"%.0f\" x2 = \"%.0f\" y2 = \"%.0f\" style = \"stroke:rgb(0,0,0);stroke-width:4\"/>",
            TL.x, TL.y, BL.x, BL.y) << endl;

        auto ptAmt = 5;// CfgValNPS("YAxisMarkers", 5);
        for (auto i = 1; i <= ptAmt; i++)
        {
            float X = (BL.x - GraphXOffset / 2);
            float Y = (BL.y - i * (ImageHeight / ptAmt / peakMargin));
            float Value = (peakf * i / ptAmt / intervalduration);
            out << Utility::Format("\t<text x=\"%.0f\" y=\"%.0f\" fill=\"black\">%.2f</text>",
                X, Y, Value) << endl;

            out << Utility::Format("\t<line x1 = \"%.0f\" y1 = \"%.0f\" x2 = \"%.0f\" y2 = \"%.0f\" style = \"stroke:rgb(0,0,0);stroke-width:0.5\"/>",
                X, Y, GraphXOffset + RealGraphWidth, Y) << endl;
        }

        for (auto point : dataPoints)
        {
            double relativeFreq = point / peak;
            double relFreqNext;
            int x1, y1, x2, y2;

            if (ptIdx + 1 < dataPoints.size())
            {
                relFreqNext = dataPoints[ptIdx + 1] / peak;
            }
            else
                relFreqNext = 0;

            x1 = IntervalWidth * ptIdx * XRatio + GraphXOffset;
            y1 = ImageHeight - ImageHeight * relativeFreq + GraphYOffset;

            x2 = IntervalWidth * (ptIdx + 1) * XRatio + GraphXOffset;
            y2 = ImageHeight - ImageHeight * relFreqNext + GraphYOffset;

            out << Utility::Format("\t<line x1 = \"%d\" y1 = \"%d\" x2 = \"%d\" y2 = \"%d\" style = \"stroke:rgb(255,0,0);stroke-width:2\"/>\n",
                x1, y1, x2, y2);

            ptIdx++;
        }

        out << "</svg>";
        return out.str();
    }
};

void ConvertToNPSGraph(rd::Song *Sng, std::filesystem::path PathOut)
{
    for (auto i = 0; i < Sng->Difficulties.size(); i++)
    {
        auto Diff = Sng->GetDifficulty(i);

		auto s = Utility::Format("%s (%s) - %s.svg", Sng->Title.c_str(), Diff->Name.c_str(), Diff->Author.c_str());
        std::filesystem::path name = PathOut / s;

		//Log::LogPrintf("Opening %s and writing graph.\n", name.string().c_str());

		std::ofstream out(name.string());
        double interv = 1;//CfgValNPS("IntervalTime", 1);
        double margin = 1.2;//CfgValNPS("PeakMargin", 1.2f);

		if (out.is_open())
			out << NPSGraph(Sng).GetSVGText(i, interv, margin);
		// else
		//	Log::LogPrintf("Couldn't open file.\n");
    }
}
