#pragma once

class SongDatabase;

class SongLoader
{
    SongDatabase* DB;

public:
    SongLoader(SongDatabase* usedDatabase);

	void LoadBMS(
		rd::Song * &BMSSong,
		std::filesystem::path File,
		std::map<std::string, rd::Song *> &bmsk,
		std::vector<rd::Song *> & VecOut);

	void LoadSong7KFromDir(std::filesystem::path songPath, std::vector<rd::Song*> &VecOut);
    void GetSongList7K(std::vector<rd::Song*> &OutVec, std::filesystem::path Dir);
    std::shared_ptr<rd::Song> LoadFromMeta(const rd::Song* Meta, std::shared_ptr<rd::Difficulty> CurrentDiff, std::filesystem::path& FilenameOut, uint8_t& Index);
};

std::shared_ptr<rd::Song> LoadSong7KFromFilename(
        const std::filesystem::path& Filename,
        rd::Song* Sng = nullptr,
        SongDatabase* DB = nullptr);