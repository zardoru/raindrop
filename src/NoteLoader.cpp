#include "Global.h"
#include "NoteLoader.h"
#include "Game_Consts.h"
#include <fstream>

float ScreenDifference()
{
	return std::abs(float((ScreenWidth / 2.f) - (PlayfieldWidth / 2.f)));
}

// func returns a positive number for x position. A negative one for a measure finish.
GameObject parse_notes(const char* str, int * len)
{
	std::string buf;
	const char* start_pt = str;
	bool isHold = false;
	GameObject retVal;

start:
	if (*str != '{')
	{
		if (*str == ',') // measure end
		{
			*len = 1; // skip one
			retVal.position.x = -1;
			return retVal;
		}

		std::stringstream serr;
		serr << "unexpected token \"" << *str << "\" while parsing";
		throw std::exception(serr.str().c_str());
	}

	str++; // move ahead one char.
	while (*str != '}' && *str != '\0' && *str != ';' && *str != ',') // , is the end of a measure.
	{
		if (*str == ' ') // basically something like {xxx yyy} will be a hold with yyy being the duration (in beats)
			isHold = true;
		buf += *str; 
		str++;
	}

	if (*str == '\0')
	{
		std::stringstream serr;
		serr << "unexpected end of line while parsing (this shouldn't happen)";
		throw std::exception(serr.str().c_str());
	}

	// we parse our value within braces..
	std::stringstream out (buf);
	int tempVal;

	// parse as iostream
	out >> tempVal;

	retVal.position.x = tempVal;

	if (isHold)
	{
		float heldTime;
		out >> heldTime;
		retVal.hold_duration = heldTime;
	}

	// skip what we parsed
	*len = str - start_pt + 1;
	return retVal;
}

Song NoteLoader::LoadObjectsFromFile(std::string filename, std::string prefix)
{
	std::ifstream filein;
	filein.open(filename.c_str(), std::ios::in);
	std::vector<GameObject> *myVec = new std::vector<GameObject>();
	Song Out;
	int Measure = 0; // Current measure
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
			Out.SongName = line.substr(line.find_first_of(":") + 1);
		}

		// Then, Timing data.
		if (command.find("#BPM") != std::string::npos)
		{
			std::stringstream str (line.substr(line.find_first_of(":") + 1));
			str >> Out.BPM;
		}

		if (command.find("#OFFSET") != std::string::npos)
		{
			std::stringstream str (line.substr(line.find_first_of(":") + 1));
			str >> Out.Offset;
		}

		// Then, file info.
		if (command.find("#SONG") != std::string::npos)
		{
			Out.SongDir = prefix + "/" + line.substr(line.find_first_of(":") + 1);
		}

		if (command.find("#BACKGROUNDIMAGE") != std::string::npos)
		{
			Out.BackgroundDir = prefix + "/" + line.substr(line.find_first_of(":") + 1);
		}

		// Then, the charts.
		if (command.find("#NOTES") != std::string::npos) // current command is notes?
		{
			// get the object string (all between a colon and a semicolon.
			std::string objectstring = line.substr(line.find_first_of(":") + 1);

			objectstring += ";"; 

			const char *process_str = objectstring.c_str();
			bool invert = false;

			while (*process_str != '\0' && *process_str != ';')
			{
				int adv_amt; // how much we advance in the string!

				while (*process_str == '\n' || *process_str == ' ') // skip whitespace
				{
					process_str++;
				}

				if (*process_str == 'M') // Horizontal mirror flag?
				{
					process_str++;
					invert = true;
				}

				GameObject obj = parse_notes(process_str, &adv_amt);

				if (obj.position.x > -1)
				{
					obj.Measure = Measure;
					obj.MeasurePos = MeasurePos;

					if (invert && obj.position.x > 0)
					{
						obj.position.x = PlayfieldWidth - obj.position.x;
					}

					if (obj.position.x > 0) // Adjust as neccesary (to fit in playing field) if it's a valid note.
						obj.position.x += ScreenDifference();

					myVec->push_back(obj);
					// We calculate beat and time later.
					MeasurePos++;
				}
				else if (obj.position.x == -1)
				{
					if (invert)
						invert = false;
					Measure++;
					MeasurePos = 0; // Reset position within measure.
				}

				process_str += adv_amt;
			} // while process_str
		}// command == #notes
	}
	
	Out.MeasureCount = Measure + 1;
	Out.Notes = myVec;
	// at this point the objects are sorted! by measure and within the measure, by fraction.
	Out.Process();

	Out.Notes = myVec;
	return Out;
}