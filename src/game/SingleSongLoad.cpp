#include <game/Song.h>
#include <game/OtowormLoaderBridge.h>
#include <game/SingleSongLoad.h>

#include <note_loader_7k.h>

std::shared_ptr<rd::Song> rd::LoadSongFromFile(std::filesystem::path filename)
{
    auto otoworm_song = otoworm::load_song_from_file(filename);
    if (!otoworm_song)
        return nullptr;

    auto song = std::make_shared<rd::Song>();
    rd::ConvertFromOtoworm(std::move(otoworm_song), song.get());
	return song;
}
