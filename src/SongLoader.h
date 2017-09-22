#pragma once

class SongDatabase;

class SongLoader
{
    SongDatabase* DB;

public:
    SongLoader(SongDatabase* usedDatabase);

    void LoadSong7KFromDir(std::filesystem::path songPath, std::vector<Game::VSRG::Song*> &VecOut);
    void GetSongList7K(std::vector<Game::VSRG::Song*> &OutVec, std::filesystem::path Dir);
    std::shared_ptr<Game::VSRG::Song> LoadFromMeta(const Game::VSRG::Song* Meta, std::shared_ptr<Game::VSRG::Difficulty> CurrentDiff, std::filesystem::path& FilenameOut, uint8_t& Index);
};

std::shared_ptr<Game::VSRG::Song> LoadSong7KFromFilename(std::filesystem::path Filename, std::filesystem::path Prefix, Game::VSRG::Song *Sng);
std::shared_ptr<Game::VSRG::Song> LoadSong7KFromFilename(const std::filesystem::path&, Game::VSRG::Song *Sng = nullptr);