#include "Global.h"
#include "NoteLoaderSM.h"
#include "NoteLoader.h"
#include <fstream>

/* Stepmania uses a format with a lot of information, so we'll start with the basics*/

using namespace NoteLoaderSM;

void LoadTracksSM(Song7K *Out, SongInternal::TDifficulty<TrackNote> *Difficulty, String line)
{
	
}

Song7K* LoadObjectsFromFile(String filename, String prefix = "")
{
	std::ifstream filein (filename.c_str());
	Song7K *Out = new Song7K();
	SongInternal::TDifficulty<TrackNote> *Difficulty = new SongInternal::TDifficulty<TrackNote>();

	if (!filein.is_open())
	{
#ifndef NDEBUG
		std::stringstream serr;
		serr << "couldn't open \"" << filename << "\" for reading";
		throw std::exception(serr.str().c_str());
#else
		return NULL;
#endif
	}

	Out->SongDirectory = prefix;

	String line;
	while (!filein.eof())
	{
		std::getline(filein, line, ';'); 
		String command = line.substr(0, line.find_first_of(":"));

#define OnCommand(x) if(command.find(#x)!=String::npos)

		String CommandContents = line.substr(line.find_first_of(":") + 1);

		OnCommand(#TITLE)
		{
			Out->SongName = CommandContents;
		}

		OnCommand(#ARTIST)
		{
			Out->SongAuthor = CommandContents;
		}

		OnCommand(#BACKGROUND)
		{
			Out->BackgroundRelativeDir = CommandContents;
			Out->BackgroundDir = prefix + "/" + CommandContents;
		}

		OnCommand(#MUSIC)
		{
			Out->SongFilename = prefix + "/" + CommandContents;
			Out->SongRelativePath = CommandContents;
		}

		OnCommand(#OFFSET)
		{
			std::stringstream str (CommandContents);
			str >> Difficulty->Offset;
			Difficulty->Offset = -Difficulty->Offset;

			Out->LeadInTime = Difficulty->Offset < 0? abs(Difficulty->Offset) + 3 : 0;
		}

		OnCommand(#BPMS)
		{
			NoteLoader::LoadBPMs(Out, Difficulty, line);
		}

		/* Stops: TBD */

		OnCommand(#NOTES)
		{
			LoadTracksSM(Out, Difficulty, line);
		}
	}
}