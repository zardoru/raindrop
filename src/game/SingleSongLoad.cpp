#include "rmath.h"

#include <game/GameConstants.h>
#include <game/Song.h>
#include <game/NoteLoader7K.h>
#include <game/SingleSongLoad.h>

#include "TextAndFileUtil.h"

const rd::loaderVSRGEntry_t LoadersVSRG[] = {
    { ".bms",   NoteLoaderBMS::LoadObjectsFromFile },
    { ".bme",   NoteLoaderBMS::LoadObjectsFromFile },
    { ".bml",   NoteLoaderBMS::LoadObjectsFromFile },
    { ".pms",   NoteLoaderBMS::LoadObjectsFromFile },
    { ".sm",    NoteLoaderSM::LoadObjectsFromFile  },
    { ".osu",   NoteLoaderOM::LoadObjectsFromFile  },
    { ".ft2",   NoteLoaderFTB::LoadObjectsFromFile },
    { ".ojn",   NoteLoaderOJN::LoadObjectsFromFile },
    { ".ssc",   NoteLoaderSSC::LoadObjectsFromFile },
    { ".bmson", NoteLoaderBMSON::LoadObjectsFromFile }
};

std::shared_ptr<rd::Song> LoadSongFromFile(std::filesystem::path filename)
{
    if (!filename.has_extension())
    {
        return nullptr;
    }

	filename = std::filesystem::absolute(filename);

    auto Sng = std::make_shared<rd::Song>();

    Sng->SongDirectory = filename.parent_path();

    for (int i = 0; i < sizeof(LoadersVSRG) / sizeof(rd::loaderVSRGEntry_t); i++)
    {
        if (filename.extension() == LoadersVSRG[i].Ext)
        {
            LoadersVSRG[i].LoadFunc(filename, Sng.get());

            auto hash = Utility::GetSha256ForFile(filename);
            auto dindex = 0;
            for (auto &d : Sng->Difficulties) {
                d->Data->FileHash = hash;
                if (d->Data->IndexInFile == -1) {
                    d->Data->IndexInFile = dindex;
                    dindex++;
                }
            }

            break;
        }
    }

	return Sng;
}