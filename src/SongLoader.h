#pragma once

class SongDatabase;

class SongLoader
{
    SongDatabase* DB;

public:
    SongLoader(SongDatabase* usedDatabase);

    void LoadSong7KFromDir(std::filesystem::path songPath, std::vector<VSRG::Song*> &VecOut);
    void LoadSongDCFromDir(std::filesystem::path songPath, std::vector<dotcur::Song*> &VecOut);
    void GetSongListDC(std::vector<dotcur::Song*> &OutVec, std::filesystem::path Dir);
    void GetSongList7K(std::vector<VSRG::Song*> &OutVec, std::filesystem::path Dir);
    std::shared_ptr<VSRG::Song> LoadFromMeta(const VSRG::Song* Meta, std::shared_ptr<VSRG::Difficulty>& CurrentDiff, std::filesystem::path& FilenameOut, uint8_t& Index);
};

std::shared_ptr<VSRG::Song> LoadSong7KFromFilename(std::filesystem::path Filename, std::filesystem::path Prefix, VSRG::Song *Sng);