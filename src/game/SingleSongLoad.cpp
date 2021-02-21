#include "rmath.h"

#include <game/GameConstants.h>
#include <game/Song.h>
#include <game/NoteLoader7K.h>
#include <game/SingleSongLoad.h>

#include "TextAndFileUtil.h"

const rd::loaderVSRGEntry_t LoadersVSRG[] = {
    { L".bms",   NoteLoaderBMS::LoadObjectsFromFile },
    { L".bme",   NoteLoaderBMS::LoadObjectsFromFile },
    { L".bml",   NoteLoaderBMS::LoadObjectsFromFile },
    { L".pms",   NoteLoaderBMS::LoadObjectsFromFile },
    { L".sm",    NoteLoaderSM::LoadObjectsFromFile  },
    { L".osu",   NoteLoaderOM::LoadObjectsFromFile  },
    { L".ft2",   NoteLoaderFTB::LoadObjectsFromFile },
    { L".ojn",   NoteLoaderOJN::LoadObjectsFromFile },
    { L".ssc",   NoteLoaderSSC::LoadObjectsFromFile },
    { L".bmson", NoteLoaderBMSON::LoadObjectsFromFile }
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