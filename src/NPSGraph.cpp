#include "GameGlobal.h"
#include "Song7K.h"

#include <sstream>
#include <fstream>
#include <boost/format.hpp>

float CfgValNPS(GString name, float defaultvalue)
{
	float ret = Configuration::GetConfigf(name, "NPS");
	if (!ret) {
		std::stringstream ss;
		ss << defaultvalue;
		Configuration::SetConfig(name, ss.str(), "NPS");
		return defaultvalue;
	}
	else {
		return ret;
	}
}

class NPSGraph {
	VSRG::Song* Song;

	int CountInterval(VSRG::Difficulty* In, double timeStart, double timeEnd)
	{
		int out = 0;
		for (auto m : In->Data->Measures) {
			for (int k = 0; k < In->Channels; k++) {
				for (auto note : m.MeasureNotes[k])	{
					if (note.StartTime >= timeStart || (note.EndTime >= timeStart && note.EndTime) ) {
						if (note.StartTime < timeEnd || (note.EndTime < timeEnd && note.EndTime)) {
							out++;
						}
					}
				}
			}
		}

		return out;
	}

public:
	NPSGraph(VSRG::Song* In)
	{
		Song = In;
	}

	vector<int> GetDataPoints(int diffIndex, double intervalduration)
	{
		vector<int> datapoints;
		VSRG::Difficulty *Diff = Song->Difficulties.at(diffIndex).get();
		if (Diff == nullptr) return datapoints;

		int intervalCount = ceil(Diff->Duration / intervalduration);
		for (int i = 0; i < intervalCount; i++)
		{
			datapoints.push_back(CountInterval(Diff, i * intervalduration, (i + 1) * intervalduration));
		}

		return datapoints;
	}

	GString GetSVGText(int diffIndex, double intervalduration = 1, double peakMargin = 1.2)
	{
		std::stringstream out;
		vector<int> dataPoints = GetDataPoints(diffIndex, intervalduration);
		auto peak_it = std::max_element(dataPoints.begin(), dataPoints.end());
		float peakf = *peak_it;
		double peak = *peak_it * peakMargin;

		out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">\n";

		auto ptIdx = 0;
		float ImageHeight = CfgValNPS("GraphHeight", 300);
		float GraphYOffset = CfgValNPS("GraphYOffs", 50);
		float GraphXOffset = CfgValNPS("GraphXOffs", 100);
		
		float IntervalWidth = 10;
		float GraphWidth = dataPoints.size() * IntervalWidth;

		float RealGraphWidth = CfgValNPS("Width", 1000);
		float XRatio = RealGraphWidth / GraphWidth;

		Vec2 BL(GraphXOffset, GraphYOffset + ImageHeight);
		Vec2 BR(RealGraphWidth, 0);
		Vec2 TL(GraphXOffset, GraphYOffset);
		BR += BL;

		GString DiffAuth = Song->GetDifficulty(diffIndex)->Author;

		if (!DiffAuth.length())
			DiffAuth = "an anonymous charter";

		float avgNPS = Song->GetDifficulty(diffIndex)->TotalScoringObjects / Song->GetDifficulty(diffIndex)->Duration;

		out << boost::format("<text x=\"%d\" y=\"%d\" fill=\"black\">%s - %s (%s) by %s (Max NPS: %.2f/Avg NPS: %.2f)</text>\n")
			% 20 % 20
			% Song->SongName % Song->SongAuthor % Song->GetDifficulty(diffIndex)->Name % DiffAuth
			% (peakf / intervalduration)
			% avgNPS;

		out << boost::format("\t<line x1 = \"%d\" y1 = \"%d\" x2 = \"%d\" y2 = \"%d\" style = \"stroke:rgb(0,0,0);stroke-width:4\"/>\n")
			% BL.x % BL.y % BR.x % BR.y;

		out << boost::format("\t<line x1 = \"%d\" y1 = \"%d\" x2 = \"%d\" y2 = \"%d\" style = \"stroke:rgb(0,0,0);stroke-width:4\"/>\n")
			% TL.x % TL.y % BL.x % BL.y;

		auto ptAmt = 5;
		for (auto i = 1; i <= ptAmt; i++)
		{
			float X = (BL.x - GraphXOffset / 2);
			float Y = (BL.y - i * (ImageHeight / ptAmt / peakMargin));
			float Value = (peakf * i / ptAmt / intervalduration);
			out << boost::format("\t<text x=\"%d\" y=\"%d\" fill=\"black\">%.2f</text>\n")
				% X
				% Y
				% Value;

			out << boost::format("\t<line x1 = \"%d\" y1 = \"%d\" x2 = \"%d\" y2 = \"%d\" style = \"stroke:rgb(0,0,0);stroke-width:0.5\"/>\n")
				% X % Y
				% (GraphXOffset + RealGraphWidth) % Y;
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
			y2 = ImageHeight - ImageHeight * relFreqNext  + GraphYOffset;

			out << (boost::format("\t<line x1 = \"%d\" y1 = \"%d\" x2 = \"%d\" y2 = \"%d\" style = \"stroke:rgb(255,0,0);stroke-width:2\"/>\n")
					% x1 % y1 % x2 % y2).str();


			ptIdx++;
		}

		out << "</svg>";
		return out.str();
	}
};

void ConvertToNPSGraph(VSRG::Song *Sng, Directory PathOut)
{
	for (int i = 0; i < Sng->Difficulties.size(); i++)
	{
		std::ofstream out;
		VSRG::Difficulty *Diff = Sng->GetDifficulty(i);
		Directory Sn = Sng->SongName;
		Sn.Normalize(true);

		GString name = (boost::format("%1%/ %4% (%2%) - %3%.svg") % PathOut.c_path() % Diff->Name % Diff->Author % Sn.c_path()).str();

		out.open(name.c_str());

		double interv = CfgValNPS("IntervalTime", 1);
		double margin = CfgValNPS("PeakMargin", 1.2f);

		out << NPSGraph(Sng).GetSVGText(i, interv, margin);
	}
}