#include "GameGlobal.h"
#include "SongDC.h"
#include "NoteLoaderDC.h"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/foreach.hpp>
#include <fstream>

/* Note Loader for the .dcf format. Heavily inspired by Stepmania. */

using namespace dotcur;

float _ScreenDifference()
{
	return std::abs(float((ScreenWidth / 2.f) - (PlayfieldWidth / 2.f)));
}

void LoadNotes(Song* Out, Difficulty * Diff, String line)
{
	// get the object string (all between a colon and a semicolon.
	String objectstring = line.substr(line.find_first_of(":") + 1);
	std::vector< String > splitvec;
	bool invert = false;

	Diff->Name = Out->SongName; // todo: change this.
	Diff->TotalNotes = Diff->TotalHolds = Diff->TotalObjects = 0;

	// Remove whitespace.
	boost::replace_all(objectstring, "\n", "");
	boost::replace_all(objectstring, "\r", "");
	// boost::replace_all(objectstring, "M", ""); // mirror flags

	boost::split(splitvec, objectstring, boost::is_any_of(",")); // Separate measures!
	BOOST_FOREACH(String objectlist, splitvec) // for each measure
	{
		std::vector< String > splitobjects;
		Measure Msr;
		invert = false;

		if ( objectlist.length() == 0 )
		{
			Diff->Measures.push_back(Msr);
			continue;
		}

		/* Mirror command. */
		if ( objectlist[0] == 'M')
		{
			invert = true;
			boost::replace_all(objectlist, "M", "");
		}

		boost::split(splitobjects, objectlist, boost::is_any_of("{}"), boost::algorithm::token_compress_on);
		size_t SoSize = 0;
		size_t CurObj = 0;

		BOOST_FOREACH (String object_description, splitobjects) // Count total valid objects
		{
			if (object_description.length() != 0)
				SoSize += 1;
		}

		BOOST_FOREACH (String object_description, splitobjects) // For all objects in measure
		{
			std::vector< String > object_parameters;

			if (object_description.length() == 0) // we must have at least a plain "0"
				continue;

			boost::split(object_parameters, object_description, boost::is_any_of(" :"));
			if (object_parameters.size() > 0) // We got a position
			{
				int32 xpos = 0;
				float hold_duration = 0;
				int32 sound = 0;

				if (object_parameters[0].length() > 0) // does it have length?
					xpos = boost::lexical_cast<float> (object_parameters[0].c_str()); // assign it
				

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
				{
					Temp.SetPositionX(xpos);
					Diff->TotalObjects++;

					if (hold_duration)
						Diff->TotalHolds++;
					else
						Diff->TotalNotes++;
				}
				else
				{
					/* Position 0 is a special X constant that will make the note invisible 
					as well as making it not emit any kind of judgment in-game. It's filler. */
					Temp.SetPositionX(0);
				}

				Temp.Assign(hold_duration, Diff->Measures.size(), (double)CurObj / SoSize);
				Msr.push_back(Temp);

			} // got a position
			CurObj++;
		} // foreach object in measure

		Diff->Measures.push_back(Msr);

	} // foreach measure

	Out->Difficulties.push_back(Diff); 
}

Song* NoteLoader::LoadObjectsFromFile(String filename, String prefix)
{
	std::ifstream filein;
	filein.open(filename.c_str(), std::ios::in);
	Song *Out = new Song();
	Difficulty *Diff = new Difficulty();

	if (!filein.is_open())
	{
#ifndef NDEBUG
		std::stringstream serr;
		serr << "couldn't open \"" << filename << "\" for reading";
		throw; // std::exception(serr.str().c_str());
#else
		return NULL;
#endif
	}

	Out->SongDirectory = prefix;

	// get lines separating with ; token
	String line;
	while (filein)
	{
		std::getline(filein, line, ';'); 
		String command = line.substr(0, line.find_first_of(":"));

#define OnCommand(x) if(command.find(#x)!=String::npos)
		
		String CommandContents = line.substr(line.find_first_of(":") + 1);

		// First, metadata.
		OnCommand(#NAME)
		{
			Out->SongName = CommandContents;
		}

		OnCommand(#AUTHOR)
		{
			Out->SongAuthor = CommandContents;
		}

		OnCommand(#MLEN)
		{
			std::stringstream str (CommandContents);
			str >> Out->MeasureLength;
		}

		// Then, Timing data.
		OnCommand(#BPM)
		{
			LoadTimingList(Diff->Timing, line);
		}

		OnCommand(#OFFSET)
		{
			std::stringstream str (CommandContents);
			str >> Diff->Offset;
			Diff->Offset += Configuration::GetConfigf("OffsetDC");
		}

		// Then, file info.
		OnCommand(#SONG)
		{
			Out->SongFilename = CommandContents;
		}

		OnCommand(#BACKGROUNDIMAGE)
		{
			Out->BackgroundFilename = CommandContents;
		}

		OnCommand(#LEADIN)
		{
			std::stringstream str (CommandContents);
			str >> Out->LeadInTime;
		}

		OnCommand(#SOUNDS)
		{
			std::vector<String> SoundList;
			String CmdLine = CommandContents;
			boost::split(SoundList, CmdLine, boost::is_any_of(","));

			for (unsigned int i = 1; i <= SoundList.size(); i++) // Copy in
				Diff->SoundList[i] = SoundList[i-1];
		}

		// Then, the charts.
		OnCommand(#NOTES) // current command is notes?
		{
			LoadNotes(Out, Diff, line);			
			Diff = new Difficulty();
		}// command == #notes
#undef OnCommand
	}
	delete Diff; // There will always be an extra copy.

	// at this point the objects are sorted! by measure and within the measure, by fraction.
	Out->Process();
	return Out;
}
