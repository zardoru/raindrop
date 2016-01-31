#pragma once

class SongDatabase;

class SongLoader
{
	SongDatabase* DB;

public:
	SongLoader(SongDatabase* usedDatabase);

	void LoadSong7KFromDir( Directory songPath, std::vector<VSRG::Song*> &VecOut );
	void LoadSongDCFromDir( Directory songPath, std::vector<dotcur::Song*> &VecOut );
	void GetSongListDC(std::vector<dotcur::Song*> &OutVec, Directory Dir);
	void GetSongList7K(std::vector<VSRG::Song*> &OutVec, Directory Dir);
	std::shared_ptr<VSRG::Song> LoadFromMeta(const VSRG::Song* Meta, shared_ptr<VSRG::Difficulty>& CurrentDiff, Directory* FilenameOut, uint8_t& Index);
};