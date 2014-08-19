#ifndef SLOADER_H_
#define SLOADER_H_

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
	VSRG::Song* LoadFromMeta(VSRG::Song* Meta, VSRG::Difficulty* CurrentDiff);
};

#endif