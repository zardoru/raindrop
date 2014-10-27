#ifndef SONGDATABASE_H_
#define SONGDATABASE_H_

struct sqlite3;
struct sqlite3_stmt;

namespace VSRG
{
	class Song;
}

class SongDatabase 
{
private:
	sqlite3 *db;
	sqlite3_stmt *st_IDQuery, 
		*st_SngInsertQuery, 
		*st_DiffInsertQuery, 
		*st_FilenameQuery, 
		*st_FilenameInsertQuery,
		*st_DiffIDQuery,
		*st_LMTQuery,
		*st_DelDiffsQuery,
		*st_GetSongInfo,
		*st_GetDiffInfo,
		*st_GetFileInfo,
		*st_UpdateLMT,
		*st_GetDiffNameQuery,
		*st_GetDiffIDFile,
		*st_DiffUpdateQuery,
		*st_GetDiffFilename;

	// Returns the ID.
	int InsertFilename(Directory Fn);
	bool DifficultyExists(int FileID, GString DifficultyName, int *IDOut = NULL);
public:

	SongDatabase(GString Database);
	~SongDatabase();

	void ClearDifficulties(int SongID);
	bool CacheNeedsRenewal(Directory Dir);
	int AddSong(Directory Dir, int Mode, Game::Song* In);
	void AddDifficulty(int SongID, Directory Filename, Game::Song::Difficulty* Diff, int Mode);
	bool IsSongDirectory(Directory Dir, int* IDOut = NULL);
	GString GetDifficultyFilename (int ID); 

	void GetSongInformation7K (int ID, VSRG::Song* Out);

	void StartTransaction();
	void EndTransaction();
};

#else
#error "SongDatabase.h included twice"
#endif