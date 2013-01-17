#include "Global.h"
#include "NoteLoader.h"
#include "Game_Consts.h"

#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <fstream>

float ScreenDifference()
{
	return std::abs(float((ScreenWidth / 2.f) - (PlayfieldWidth / 2.f)));
}

Song* NoteLoader::LoadObjectsFromFile(std::string filename, std::string prefix)
{
	std::ifstream filein;
	filein.open(filename.c_str(), std::ios::in);
	std::vector<GameObject> *myVec = new std::vector<GameObject>();
	Song *Out = new Song();
	int Measure = -1; // Current measure
	int MeasurePos = 0; // position within the measure. (we divide later by measure * mlen + measure fraction to get current beat)

	if (!filein.is_open())
	{
		std::stringstream serr;
		serr << "couldn't open \"" << filename << "\" for reading";
		throw std::exception(serr.str().c_str());
	}

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

		// Then, Timing data.
		if (command.find("#BPM") != std::string::npos)
		{
			std::stringstream str (line.substr(line.find_first_of(":") + 1));
			str >> Out->BPM;
		}

		if (command.find("#OFFSET") != std::string::npos)
		{
			std::stringstream str (line.substr(line.find_first_of(":") + 1));
			str >> Out->Offset;
		}

		// Then, file info.
		if (command.find("#SONG") != std::string::npos)
		{
			Out->SongDir = prefix + "/" + line.substr(line.find_first_of(":") + 1);
		}

		if (command.find("#BACKGROUNDIMAGE") != std::string::npos)
		{
			Out->BackgroundDir = prefix + "/" + line.substr(line.find_first_of(":") + 1);
		}

		// Then, the charts.
		if (command.find("#NOTES") != std::string::npos) // current command is notes?
		{
			// get the object string (all between a colon and a semicolon.
			std::string objectstring = line.substr(line.find_first_of(":") + 1);
			std::vector< std::string > splitvec;
			bool invert = false;

			// Remove whitespace.
			boost::replace_all(objectstring, "\n", "");
			// boost::replace_all(objectstring, "M", ""); // mirror flags

			boost::split(splitvec, objectstring, boost::is_any_of(",")); // Separate measures!
			BOOST_FOREACH(std::string objectlist, splitvec)
			{
				std::vector< std::string > splitobjects;
				invert = false;

				if ( objectlist.length() == 0 )
				{
					Measure++;
					continue;
				}

				if ( objectlist.at(0) == 'M')
				{
					invert = true;
					boost::replace_all(objectlist, "M", "");
				}

				Measure++;
				MeasurePos = 0;

				boost::split(splitobjects, objectlist, boost::is_any_of("{}"), boost::algorithm::token_compress_on);
				BOOST_FOREACH (std::string object_description, splitobjects) // For all objects in measure
				{
					std::vector< std::string > object_parameters;

					if (object_description.length() == 0) // we must have at least a plain "0"
						continue;

					boost::split(object_parameters, object_description, boost::is_any_of(" :"));
					if (object_parameters.size() > 0) // We got a position
					{
						int xpos = 0;
						float hold_duration = 0;
						int sound = 0;

						if (object_parameters[0].length() > 0) // does it have length?
							xpos = boost::lexical_cast<int> (object_parameters[0].c_str()); // assign it

						if (object_parameters.size() > 1) // We got a hold note parameter
						{
							if (object_parameters[1].length() > 0) // length?
								hold_duration = boost::lexical_cast<float> (object_parameters[1].c_str()); // load it in

							if (object_parameters.size() > 2) // We got a sound parameter
							{
								if (object_parameters[2].length() > 0) // got a valid sound?
									sound = boost::lexical_cast<int> (object_parameters[2].c_str()); // cast it in
							}
						}

						if (invert)
						{
							if (xpos != 0)
								xpos = PlayfieldWidth - xpos;
						}

						GameObject Temp;

						if (xpos != 0)
							Temp.position.x = xpos + ScreenDifference();
						else
							Temp.position.x = 0;

						Temp.hold_duration = hold_duration;
						Temp.Measure = Measure;
						Temp.MeasurePos = MeasurePos;
						myVec->push_back(Temp);

					}
					MeasurePos++;
				}
			}
		}// command == #notes
	}
	
	Out->MeasureCount = Measure + 1;
	Out->Notes = myVec;
	// at this point the objects are sorted! by measure and within the measure, by fraction.
	Out->Process();

	Out->Notes = myVec;
	return Out;
}