#include "pch.h"

#include "GameGlobal.h"
#include "SongDC.h"
#include "NoteLoaderDC.h"

/* Note Loader for the .dcf format. Heavily inspired by Stepmania. */
/* Even for dotcur, I wouldn't use this anymore, I just keep it for historical purposes. I'd use bmson. */

using namespace dotcur;

float _ScreenDifference()
{
	return std::abs(float((ScreenWidth / 2.f) - (PlayfieldWidth / 2.f)));
}

void LoadNotes(Song* Out, Difficulty * Diff, GString line)
{
	// get the object GString (all between a colon and a semicolon.
	GString objectString = line.substr(line.find_first_of(":") + 1);
	std::vector< GString > splitvec;
	bool invert = false;

	Diff->Name = Out->SongName; // todo: change this.
	Diff->TotalNotes = Diff->TotalHolds = Diff->TotalObjects = 0;

	// Remove whitespace.
	Utility::ReplaceAll(objectString, "[\n\r]", "");

	splitvec = Utility::TokenSplit(objectString); // Separate measures!
	for(GString objectlist: splitvec) // for each measure
	{
		std::vector< GString > splitobjects;
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
			Utility::ReplaceAll(objectlist, "M", "");
		}

		splitobjects = Utility::TokenSplit(objectlist, "{}", true);
		size_t SoSize = 0;
		size_t CurObj = 0;

		for(GString object_description: splitobjects) // Count total valid objects
		{
			if (object_description.length() != 0)
				SoSize += 1;
		}

		for(GString object_description: splitobjects) // For all objects in measure
		{
			std::vector< GString > object_parameters;

			if (object_description.length() == 0) // we must have at least a plain "0"
				continue;

			object_parameters = Utility::TokenSplit(object_description, " :");
			if (object_parameters.size() > 0) // We got a position
			{
				int32 xpos = 0;
				float hold_duration = 0;
				int32 sound = 0;

				if (object_parameters[0].length() > 0) // does it have length?
					xpos = latof (object_parameters[0].c_str()); // assign it
				

				if (object_parameters.size() > 1) // We got a hold note parameter
				{
					if (object_parameters[1].length() > 0) // length?
						hold_duration = latof (object_parameters[1].c_str()); // load it in

					if (object_parameters.size() > 2) // We got a sound parameter
					{
						if (object_parameters[2].length() > 0) // got a valid sound?
							sound = latof (object_parameters[2].c_str()); // cast it in
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

Song* NoteLoader::LoadObjectsFromFile(GString filename, GString prefix)
{
	std::ifstream filein;
	filein.open(filename.c_str(), std::ios::in);
	Song *Out = new Song();
	Difficulty *Diff = new Difficulty();

	if (!filein.is_open())
	{
		throw std::exception(Utility::Format("Unable to open %s for reading!", filename.c_str()).c_str());
	}

	Out->SongDirectory = prefix;

	// get lines separating with ; token
	GString line;
	while (filein)
	{
		std::getline(filein, line, ';'); 
		GString command = line.substr(0, line.find_first_of(":"));

#define OnCommand(x) if(command.find(#x)!=GString::npos)
		
		GString CommandContents = line.substr(line.find_first_of(":") + 1);

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
            std::vector<GString> SoundList;
			GString CmdLine = CommandContents;
			// Diff->SoundList = Utility::TokenSplit(CmdLine, ",");
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
