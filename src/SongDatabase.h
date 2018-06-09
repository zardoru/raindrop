#pragma once

struct sqlite3;
struct sqlite3_stmt;

namespace Game {
	namespace VSRG
	{
		class Song;
	}
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
        *stGetLMTQuery,
        *st_DelDiffsQuery,
        *stGetSongInfo,
        *stGetDiffInfo,
        *st_GetFileInfo,
        *st_UpdateLMT,
        *st_GetDiffIDFile,
        *st_DiffUpdateQuery,
        *st_GetDiffFilename,
        *st_GetSIDFromFilename,
        *st_GetLastSongID,
        *st_GetDiffAuthor,
		*st_GetDiffGenre,
        *st_GetPreviewInfo,
        *st_GetStageFile,
		*st_FileFromHash,
		*st_HashFromFile;

    // Returns the ID.
    int InsertOrUpdateChartFile(Game::VSRG::Difficulty* Diff);
    bool DifficultyExists(int FileID, std::string DifficultyName, int *IDOut = NULL);

	void UpdateDiffInternal(int &ret, int DiffID, Game::Song::Difficulty * Diff);
	void InsertDiffInternal(int &ret, int SongID, int FileID, Game::Song::Difficulty * Diff);
	int InsertSongInternal(Game::VSRG::Song *Song);
public:

    SongDatabase(std::string Database);
    ~SongDatabase();

    void ClearDifficulties(int SongID);
    bool CacheNeedsRenewal(std::filesystem::path Dir);

	/* 
		Adds a difficulty to the database, or updates it if it already exists. 
		The difficulty gets its ID from here.
	*/
	void InsertOrUpdateDifficulty(int SongID, Game::VSRG::Difficulty* Diff);

	/*
		Sets the song ID to one corresponding on the database
		as well as sets the song ID for faster further 
		querying with the database, once the non-metadata is removed.
	*/
	void AssociateSong(Game::VSRG::Song * New);

    void GetPreviewInfo(int SongID, std::string &Filename, float &PreviewStart);

    // Difficulty information
    std::filesystem::path GetDifficultyFilename(int DiffID);

	// hashes
	std::filesystem::path GetChartFilename(std::string hash);
	std::string GetChartHash(std::filesystem::path filename);

    std::string GetArtistForDifficulty(int DiffID);
	std::string GetGenreForDifficulty(int DiffID);
    std::string GetStageFile(int DiffID);

    int GetSongIDForFile(std::filesystem::path File);

	/*
		Load most song metadata.
		Excludes:
		- SongDirectory
		- BackgroundFilename
		- SongPreviewSource
		- Genre
		- Difficulty data (metadata only)

	*/
    void GetSongInformation(int ID, Game::VSRG::Song* Out);

    void StartTransaction();
    void EndTransaction();
};