#include "Global.h"
#include "NoteLoader.h"
#include "Game_Consts.h"
#include "Audio.h"

#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <fstream>

float _ScreenDifference()
{
	return std::abs(float((ScreenWidth / 2.f) - (PlayfieldWidth / 2.f)));
}

Song* NoteLoader::LoadObjectsFromFile(std::string filename, std::string prefix)
{
	std::ifstream filein;
	filein.open(filename.c_str(), std::ios::in);
	Song *Out = new Song();
	SongInternal::Difficulty *Difficulty = new SongInternal::Difficulty();
	int32 Measure = -1; // Current measure
	int32 MeasurePos = 0; // position within the measure. (we divide later by measure * mlen + measure fraction to get current beat)

	if (!filein.is_open())
	{
		std::stringstream serr;
		serr << "couldn't open \"" << filename << "\" for reading";
		throw std::exception(serr.str().c_str());
	}

	Out->SongDirectory = prefix;

	// get lines separating with ; token
	std::string line;
	while (!filein.eof())
	{
		std::getline(filein, line, ';'); 
		std::string command = line.substr(0, line.find_first_of(":"));

		// First, metadata.
		if (command.find("#NAME") != std::string::npos)
		{
			Out->SongName = line.substr(line.find_first_of(":") + 1);
		}

		if (command.find("#AUTHOR") != std::string::npos)
		{
			Out->SongAuthor = line.substr(line.find_first_of(":") + 1);
		}

		// Then, Timing data.
		if (command.find("#BPM") != std::string::npos)
		{
			/* TODO: List based parsing */
			std::stringstream str (line.substr(line.find_first_of(":") + 1));
			SongInternal::Difficulty::TimingSegment Segment;
			str >> Segment.Value;
			Segment.Time = 0;
			Difficulty->Timing.push_back(Segment);	
		}

		if (command.find("#OFFSET") != std::string::npos)
		{
			std::stringstream str (line.substr(line.find_first_of(":") + 1));
			str >> Difficulty->Offset;
			// Difficulty->Offset += GetDeviceLatency();
		}

		// Then, file info.
		if (command.find("#SONG") != std::string::npos)
		{
			Out->SongFilename = prefix + "/" + line.substr(line.find_first_of(":") + 1);
			Out->SongRelativePath = line.substr(line.find_first_of(":") + 1);
		}

		if (command.find("#BACKGROUNDIMAGE") != std::string::npos)
		{
			Out->BackgroundDir = prefix + "/" + line.substr(line.find_first_of(":") + 1);
			Out->BackgroundRelativeDir = line.substr(line.find_first_of(":") + 1);
		}

		if (command.find("#LEADIN") != std::string::npos)
		{
			std::stringstream str;
			str << line.substr(line.find_first_of(":") + 1);
			str >> Out->LeadInTime;
		}

		// not yet
		/*
		if (command.find("#BGALUA") != std::string::npos)
		{
		}
		*/

		// Then, the charts.
		if (command.find("#NOTES") != std::string::npos) // current command is notes?
		{
			// get the object string (all between a colon and a semicolon.
			std::string objectstring = line.substr(line.find_first_of(":") + 1);
			std::vector< std::string > splitvec;
			bool invert = false;

			Difficulty->Name = Out->SongName; // todo: change this.

			// Remove whitespace.
			boost::replace_all(objectstring, "\n", "");
			// boost::replace_all(objectstring, "M", ""); // mirror flags

			boost::split(splitvec, objectstring, boost::is_any_of(",")); // Separate measures!
			BOOST_FOREACH(std::string objectlist, splitvec) // for each measure
			{
				std::vector< std::string > splitobjects;
				SongInternal::Measure Measure;
				invert = false;

				Measure.Fraction = 0;

				if ( objectlist.length() == 0 )
				{
					Difficulty->Measures.push_back(Measure);
					continue;
				}

				if ( objectlist.at(0) == 'M')
				{
					invert = true;
					boost::replace_all(objectlist, "M", "");
				}

				boost::split(splitobjects, objectlist, boost::is_any_of("{}"), boost::algorithm::token_compress_on);
				BOOST_FOREACH (std::string object_description, splitobjects) // For all objects in measure
				{
					std::vector< std::string > object_parameters;

					if (object_description.length() == 0) // we must have at least a plain "0"
						continue;

					boost::split(object_parameters, object_description, boost::is_any_of(" :"));
					if (object_parameters.size() > 0) // We got a position
					{
						int32 xpos = 0;
						float hold_duration = 0;
						int32 sound = 0;

						if (object_parameters[0].length() > 0) // does it have length?
							xpos = boost::lexical_cast<int32> (object_parameters[0].c_str()); // assign it

						if (object_parameters.size() > 1) // We got a hold note parameter
						{
							if (object_parameters[1].length() > 0) // length?
								hold_duration = boost::lexical_cast<float> (object_parameters[1].c_str()); // load it in

							if (object_parameters.size() > 2) // We got a sound parameter
							{
								if (object_parameters[2].length() > 0) // got a valid sound?
									sound = boost::lexical_cast<int32> (object_parameters[2].c_str()); // cast it in
							}
						}

						if (invert)
						{
							if (xpos != 0)
								xpos = PlayfieldWidth - xpos;
						}

						GameObject Temp;

						if (xpos != 0)
							Temp.SetPositionX(xpos);
						else
						{
							//Measure.Fraction++;
							//continue; // Don't add this note.
							// why add this note? basically the editor wont work otherwise. 
							Temp.SetPositionX(0);
						}

						Temp.hold_duration = hold_duration;
						Temp.Measure = Difficulty->Measures.size();
						Temp.MeasurePos = Measure.Fraction;
						Measure.MeasureNotes.push_back(Temp);

					} // got a position
					Measure.Fraction++;
				} // foreach object in measure

				Difficulty->Measures.push_back(Measure);

			} // foreach measure

			// A fairly expensive copy, I'd dare say?
			// However, it's loading. I don't think some delay will fuck it up that bad.
			Out->Difficulties.push_back(Difficulty); 
			Difficulty = new SongInternal::Difficulty();
		}// command == #notes
	}

	delete Difficulty; // There will always be an extra copy.

	// at this point the objects are sorted! by measure and within the measure, by fraction.
	Out->Process();
	return Out;
}