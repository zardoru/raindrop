#include "Global.h"
#include "NoteLoader.h"
#include "Game_Consts.h"
#include "Audio.h"
#include "Configuration.h"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/foreach.hpp>
#include <fstream>

/* Note Loader for the .dcf format. Heavily inspired by Stepmania. */

float _ScreenDifference()
{
	return std::abs(float((ScreenWidth / 2.f) - (PlayfieldWidth / 2.f)));
}

void LoadNotes(SongDC* Out, SongInternal::TDifficulty<GameObject>* Difficulty, String line)
{
	// get the object string (all between a colon and a semicolon.
	String objectstring = line.substr(line.find_first_of(":") + 1);
	std::vector< String > splitvec;
	bool invert = false;

	Difficulty->Name = Out->SongName; // todo: change this.
	Difficulty->TotalNotes = Difficulty->TotalHolds = Difficulty->TotalObjects = 0;

	// Remove whitespace.
	boost::replace_all(objectstring, "\n", "");
	boost::replace_all(objectstring, "\r", "");
	// boost::replace_all(objectstring, "M", ""); // mirror flags

	boost::split(splitvec, objectstring, boost::is_any_of(",")); // Separate measures!
	BOOST_FOREACH(String objectlist, splitvec) // for each measure
	{
		std::vector< String > splitobjects;
		SongInternal::Measure<GameObject> Measure;
		invert = false;

		Measure.Fraction = 0;

		if ( objectlist.length() == 0 )
		{
			Difficulty->Measures.push_back(Measure);
			continue;
		}

		/* Mirror command. */
		if ( objectlist[0] == 'M')
		{
			invert = true;
			boost::replace_all(objectlist, "M", "");
		}

		boost::split(splitobjects, objectlist, boost::is_any_of("{}"), boost::algorithm::token_compress_on);
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
					Difficulty->TotalObjects++;

					if (hold_duration)
						Difficulty->TotalHolds++;
					else
						Difficulty->TotalNotes++;
				}
				else
				{
					/* Position 0 is a special X constant that will make the note invisible 
					as well as making it not emit any kind of judgement in-game. It's filler. */
					Temp.SetPositionX(0);
				}

				Temp.Assign(hold_duration, Difficulty->Measures.size(), Measure.Fraction);
				Measure.MeasureNotes.push_back(Temp);

			} // got a position
			Measure.Fraction++;
		} // foreach object in measure

		Difficulty->Measures.push_back(Measure);

	} // foreach measure

	// A fairly expensive copy, I'd dare say?
	// However, it's loading. I don't think some delay will fuck it up that bad.
	

	Out->Difficulties.push_back(Difficulty); 
}

SongDC* NoteLoader::LoadObjectsFromFile(String filename, String prefix)
{
	std::ifstream filein;
	filein.open(filename.c_str(), std::ios::in);
	SongDC *Out = new SongDC();
	SongInternal::TDifficulty<GameObject> *Difficulty = new SongInternal::TDifficulty<GameObject>();

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
			LoadTimingList(Difficulty->Timing, line);
		}

		OnCommand(#OFFSET)
		{
			std::stringstream str (CommandContents);
			str >> Difficulty->Offset;
			Difficulty->Offset += Configuration::GetConfigf("OffsetDC");

			/*if (Configuration::GetConfigf("AudioCompensation"))
				Difficulty->Offset += GetDeviceLatency(); */
		}

		// Then, file info.
		OnCommand(#SONG)
		{
			Out->SongFilename = prefix + "/" + CommandContents;
			Out->SongRelativePath = CommandContents;
		}

		OnCommand(#BACKGROUNDIMAGE)
		{
			Out->BackgroundDir = prefix + "/" + CommandContents;
			Out->BackgroundRelativeDir = CommandContents;
		}

		OnCommand(#LEADIN)
		{
			std::stringstream str (CommandContents);
			str >> Out->LeadInTime;
		}

		OnCommand(#SOUNDS)
		{
			String CmdLine = CommandContents;
			boost::split(Difficulty->SoundList, CmdLine, boost::is_any_of(","));
		}

		// Then, the charts.
		OnCommand(#NOTES) // current command is notes?
		{
			LoadNotes(Out, Difficulty, line);			
			Difficulty = new SongInternal::TDifficulty<GameObject>();
		}// command == #notes
#undef OnCommand
	}
	delete Difficulty; // There will always be an extra copy.

	// at this point the objects are sorted! by measure and within the measure, by fraction.
	Out->Process();
	return Out;
}
