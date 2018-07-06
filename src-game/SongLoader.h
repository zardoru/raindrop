#pragma once

class SongDatabase;

class SongLoader
{
    SongDatabase* DB;

public:
    SongLoader(SongDatabase* usedDatabase);

	void LoadBMS(
		Game::VSRG::Song * &BMSSong,
		std::filesystem::path &SongDirectory,
		std::filesystem::path File,
		std::map<std::string, Game::VSRG::Song *> &bmsk,
		std::vector<Game::VSRG::Song *> & VecOut);

	void LoadSong7KFromDir(std::filesystem::path songPath, std::vector<Game::VSRG::Song*> &VecOut);
    void GetSongList7K(std::vector<Game::VSRG::Song*> &OutVec, std::filesystem::path Dir);
    std::shared_ptr<Game::VSRG::Song> LoadFromMeta(const Game::VSRG::Song* Meta, std::shared_ptr<Game::VSRG::Difficulty> CurrentDiff, std::filesystem::path& FilenameOut, uint8_t& Index);
};

std::shared_ptr<Game::VSRG::Song> LoadSong7KFromFilename(
	std::filesystem::path path
);