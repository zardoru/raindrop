#include "pch.h"


#include "Logging.h"

#include "Song7K.h"
#include "SongDatabase.h"

auto DatabaseQuery =
"CREATE TABLE IF NOT EXISTS [songdb] (\
  [id] INTEGER, \
  [songtitle] varchar(260), \
  [songauthor] varchar(260), \
  [subtitle] varchar(260), \
  [songfilename] varchar(260), \
  [songbackground] varchar(260), \
  [previewsong] varchar(260), \
  [previewtime] float, \
  PRIMARY KEY ([id]));\
CREATE TABLE IF NOT EXISTS [songfiledb] (\
  [id] INTEGER PRIMARY KEY, \
  [filename] varchar(260), \
  [lastmodified] INTEGER, \
  [hash] varchar(64)); \
CREATE TABLE IF NOT EXISTS [diffdb] (\
  [songid] INTEGER CONSTRAINT [sid] REFERENCES [songdb]([id]) ON DELETE CASCADE, \
  [diffid] INTEGER PRIMARY KEY, \
  [fileid] INT CONSTRAINT [fid] REFERENCES [songfiledb]([id]) ON DELETE CASCADE, \
  [name] VARCHAR(260), \
  [objcount] INT, \
  [scoreobjectcount] INT, \
  [duration] DOUBLE, \
  [isvirtual] INTEGER, \
  [keys] INTEGER,\
  [level] INT64,\
  [author] VARCHAR(256),\
  [genre] VARCHAR(256),\
  [stagefile] varchar(260));\
	CREATE INDEX IF NOT EXISTS diff_index ON diffdb(diffid, songid, fileid);\
	CREATE INDEX IF NOT EXISTS diff_index2 ON diffdb(songid);\
	CREATE INDEX IF NOT EXISTS diff_index3 ON diffdb(fileid);\
	CREATE INDEX IF NOT EXISTS songid_index ON songdb(id);\
	CREATE INDEX IF NOT EXISTS file_index ON songfiledb(filename);\
	CREATE INDEX IF NOT EXISTS file_index2 ON songfiledb(hash);\
	CREATE INDEX IF NOT EXISTS file_index3 ON songfiledb(id);\
  ";

auto InsertSongQuery = "INSERT INTO songdb VALUES (NULL,$title,$author,$subtitle,$fn,$bg,$psong,$ptime)";

auto InsertDifficultyQuery = "INSERT INTO diffdb VALUES (\
	$sid,NULL,$fid,$name,\
    $objcnt,$scoreobjcnt,\
    $dur,$virtual,$keys,$level,\
    $author,$genre,$stagefile)";

auto GetFilenameIDQuery = "SELECT id, lastmodified FROM songfiledb WHERE filename=$fn";

auto InsertFilenameQuery = "INSERT INTO songfiledb VALUES (NULL,$fn,$lmt,$hash)";

auto GetDiffNameQuery = "SELECT name FROM diffdb \
							 WHERE (diffdb.fileid = (SELECT songfiledb.id FROM songfiledb WHERE filename=?))";

auto GetLMTQuery = "SELECT lastmodified FROM songfiledb WHERE filename=$fn";

auto GetSongInfo = "SELECT songtitle, \
				   songauthor, songfilename,\
				   subtitle, songbackground, \
				   previewtime FROM songdb WHERE id=$sid";

auto GetDiffInfo = "SELECT diffid, name, objcount,\
				   scoreobjectcount, duration, isvirtual, \
				   keys, fileid, level, genre FROM diffdb WHERE songid=$sid";

auto GetFileInfo = "SELECT filename, lastmodified FROM songfiledb WHERE id=$fid";

auto UpdateLMT = "UPDATE songfiledb SET lastmodified=$lmt, hash=$hash WHERE filename=$fn";

auto UpdateDiff = "UPDATE diffdb SET name=$name,objcount=$objcnt,scoreobjectcount=$scoreobjcnt,\
	duration=$dur,\
	isvirtual=$virtual,\
	keys=$keys,\
	level=$level,\
	author=$author,\
	stagefile=$stagefile,\
	genre=$genre\
	WHERE diffid=$did";

auto GetDiffFilename = "SELECT filename FROM songfiledb WHERE\
 (songfiledb.id = (SELECT diffdb.fileid FROM diffdb WHERE diffid=$did))";

auto GetDiffIDFileID = "SELECT diffid FROM diffdb \
						WHERE diffdb.fileid=$fid AND\
						diffdb.name = $name";

auto GetSongIDFromFilename = "SELECT songid FROM diffdb WHERE\
 (diffdb.fileid = (SELECT id FROM songfiledb WHERE filename=$fn))";
auto GetLatestSongID = "SELECT MAX(id) FROM songdb";
auto GetAuthorOfDifficulty = "SELECT author FROM diffdb WHERE diffid=$did";
auto GetPreviewOfSong = "SELECT previewsong, previewtime FROM songdb WHERE id=$sid";
auto sGetStageFile = "SELECT stagefile FROM diffdb WHERE diffid=$did";
auto GetGenre = "SELECT genre FROM diffdb WHERE diffid=$did";

auto FileFromHash = "SELECT filename FROM songfiledb WHERE hash=$hash";
auto HashFromFile = "SELECT hash FROM songfiledb WHERE filename=$filename";

// sqlite check
#define SC(x) \
{ret=x; if(ret!=SQLITE_OK && ret != SQLITE_DONE) \
{Log::Printf("sqlite: %ls (code %d)\n",Utility::Widen(sqlite3_errmsg(db)).c_str(), ret); Utility::DebugBreak(); }}

// sqlite check sequence
#define SCS(x) \
{ret=x; if(ret!=SQLITE_DONE && ret != SQLITE_ROW) \
{Log::Printf("sqlite: %ls (code %d)\n",Utility::Widen(sqlite3_errmsg(db)).c_str(), ret); Utility::DebugBreak(); }}

SongDatabase::SongDatabase(std::string Database)
{
	int ret = sqlite3_open_v2(Database.c_str(), &db, SQLITE_OPEN_FULLMUTEX | SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);

	if (ret != SQLITE_OK)
		Log::Printf("Unable to open song database file.");
	else
	{
		char* err; // Do the "create tables" query.
		const char* tail;
		SC(sqlite3_exec(db, DatabaseQuery, NULL, NULL, &err));

		// And not just that, also the statements.
		SC(sqlite3_prepare_v2(db, InsertSongQuery, strlen(InsertSongQuery), &st_SngInsertQuery, &tail));
		SC(sqlite3_prepare_v2(db, InsertDifficultyQuery, strlen(InsertDifficultyQuery), &st_DiffInsertQuery, &tail));
		SC(sqlite3_prepare_v2(db, GetFilenameIDQuery, strlen(GetFilenameIDQuery), &st_FilenameQuery, &tail));
		SC(sqlite3_prepare_v2(db, InsertFilenameQuery, strlen(InsertFilenameQuery), &st_FilenameInsertQuery, &tail));
		SC(sqlite3_prepare_v2(db, GetLMTQuery, strlen(GetLMTQuery), &stGetLMTQuery, &tail));
		SC(sqlite3_prepare_v2(db, GetSongInfo, strlen(GetSongInfo), &stGetSongInfo, &tail));
		SC(sqlite3_prepare_v2(db, GetDiffInfo, strlen(GetDiffInfo), &stGetDiffInfo, &tail));
		SC(sqlite3_prepare_v2(db, GetFileInfo, strlen(GetFileInfo), &st_GetFileInfo, &tail));
		SC(sqlite3_prepare_v2(db, UpdateLMT, strlen(UpdateLMT), &st_UpdateLMT, &tail));
		SC(sqlite3_prepare_v2(db, GetDiffIDFileID, strlen(GetDiffIDFileID), &st_GetDiffIDFile, &tail));
		SC(sqlite3_prepare_v2(db, UpdateDiff, strlen(UpdateDiff), &st_DiffUpdateQuery, &tail));
		SC(sqlite3_prepare_v2(db, GetDiffFilename, strlen(GetDiffFilename), &st_GetDiffFilename, &tail));
		SC(sqlite3_prepare_v2(db, GetSongIDFromFilename, strlen(GetSongIDFromFilename), &st_GetSIDFromFilename, &tail));
		SC(sqlite3_prepare_v2(db, GetLatestSongID, strlen(GetLatestSongID), &st_GetLastSongID, &tail));
		SC(sqlite3_prepare_v2(db, GetAuthorOfDifficulty, strlen(GetAuthorOfDifficulty), &st_GetDiffAuthor, &tail));
		SC(sqlite3_prepare_v2(db, GetPreviewOfSong, strlen(GetPreviewOfSong), &st_GetPreviewInfo, &tail));
		SC(sqlite3_prepare_v2(db, sGetStageFile, strlen(sGetStageFile), &st_GetStageFile, &tail));
		SC(sqlite3_prepare_v2(db, GetGenre, strlen(GetGenre), &st_GetDiffGenre, &tail));
		SC(sqlite3_prepare_v2(db, FileFromHash, strlen(FileFromHash), &st_FileFromHash, &tail));
		SC(sqlite3_prepare_v2(db, HashFromFile, strlen(HashFromFile), &st_HashFromFile, &tail));
	}
}

SongDatabase::~SongDatabase()
{
	if (db)
	{
		sqlite3_finalize(st_SngInsertQuery);
		sqlite3_finalize(st_DiffInsertQuery);
		sqlite3_finalize(st_FilenameQuery);
		sqlite3_finalize(st_FilenameInsertQuery);
		sqlite3_finalize(stGetLMTQuery);
		sqlite3_finalize(stGetSongInfo);
		sqlite3_finalize(stGetDiffInfo);
		sqlite3_finalize(st_UpdateLMT);
		sqlite3_finalize(st_GetDiffIDFile);
		sqlite3_finalize(st_DiffUpdateQuery);
		sqlite3_finalize(st_GetDiffFilename);
		sqlite3_finalize(st_GetSIDFromFilename);
		sqlite3_finalize(st_GetLastSongID);
		sqlite3_finalize(st_GetDiffAuthor);
		sqlite3_finalize(st_GetPreviewInfo);
		sqlite3_finalize(st_GetStageFile);
		sqlite3_finalize(st_GetDiffGenre);
		sqlite3_finalize(st_FileFromHash);
		sqlite3_finalize(st_HashFromFile);
		sqlite3_close(db);
	}
}

// Inserts a filename, if it already exists, updates it.
// Returns the ID of the filename.
int SongDatabase::InsertOrUpdateChartFile(Game::VSRG::Difficulty* Diff)
{
	int ret;
	int idOut;
	int lmt;
	auto Fn = Diff->Filename;
	std::string u8p = Utility::ToU8(std::filesystem::absolute(Fn).wstring());

	SC(sqlite3_bind_text(st_FilenameQuery, 1, u8p.c_str(), u8p.length(), SQLITE_STATIC));

	if (sqlite3_step(st_FilenameQuery) == SQLITE_ROW)
	{
		idOut = sqlite3_column_int(st_FilenameQuery, 0);
		lmt = sqlite3_column_int(st_FilenameQuery, 1);

		int lastLmt = Utility::GetLastModifiedTime(Fn);

		// Update the last-modified-time of this file, and its hash if it has changed.
		if (lmt != lastLmt)
		{
			std::string Hash = Diff->Data->FileHash;

			SC(sqlite3_bind_int(
				st_UpdateLMT,
				sqlite3_bind_parameter_index(st_UpdateLMT, "$lmt"),
				lastLmt
			));

			SC(sqlite3_bind_text(
				st_UpdateLMT,
				sqlite3_bind_parameter_index(st_UpdateLMT, "$hash"),
				Hash.c_str(),
				Hash.length(), 
				SQLITE_STATIC
			));

			SC(sqlite3_bind_text(
				st_UpdateLMT,
				sqlite3_bind_parameter_index(st_UpdateLMT, "$fn"),
				u8p.c_str(), 
				u8p.length(),
				SQLITE_STATIC
			));

			SCS(sqlite3_step(st_UpdateLMT));
			SC(sqlite3_reset(st_UpdateLMT));
		}
	}
	else
	{
		std::string Hash = Utility::GetSha256ForFile(Fn);

		// There's no entry, got to insert it.
		SC(sqlite3_bind_text(
			st_FilenameInsertQuery,
			sqlite3_bind_parameter_index(st_FilenameInsertQuery, "$fn"),
			u8p.c_str(), 
			u8p.length(), 
			SQLITE_STATIC
		));

		SC(sqlite3_bind_int(
			st_FilenameInsertQuery,
			sqlite3_bind_parameter_index(st_FilenameInsertQuery, "$lmt"),
			Utility::GetLastModifiedTime(Fn)
		));

		SC(sqlite3_bind_text(
			st_FilenameInsertQuery,
			sqlite3_bind_parameter_index(st_FilenameInsertQuery, "$hash"),
			Hash.c_str(), 
			Hash.length(), 
			SQLITE_STATIC
		));

		// This should not fail. Otherwise, there are bigger problems to worry about...
		SCS(sqlite3_step(st_FilenameInsertQuery));
		SC(sqlite3_reset(st_FilenameInsertQuery));

		// okay, then return the ID.
		SC(sqlite3_reset(st_FilenameQuery));
		SC(sqlite3_bind_text(
			st_FilenameQuery,
			sqlite3_bind_parameter_index(st_FilenameQuery, "$fn"),
			u8p.c_str(), 
			u8p.length(),
			SQLITE_STATIC
		));

		sqlite3_step(st_FilenameQuery);
		idOut = sqlite3_column_int(st_FilenameQuery, 0);
	}

	SC(sqlite3_reset(st_FilenameQuery));
	return idOut;
}

// remove all difficulties associated to this ID
void SongDatabase::ClearDifficulties(int SongID)
{
	int ret;
	SC(sqlite3_bind_int(st_DelDiffsQuery, 1, SongID));
	SCS(sqlite3_step(st_DelDiffsQuery));
	SC(sqlite3_reset(st_DelDiffsQuery));
}

// returns if difficulty exists in the database. And difficulty ID.
bool SongDatabase::DifficultyExists(int FileID, std::string DifficultyName, int *IDOut)
{
	int ret;

	SC(sqlite3_bind_int(
		st_GetDiffIDFile, 
		sqlite3_bind_parameter_index(st_GetDiffIDFile, "$fid"), 
		FileID
	));

	SC(sqlite3_bind_text(
		st_GetDiffIDFile, 
		sqlite3_bind_parameter_index(st_GetDiffIDFile, "$name"),
		DifficultyName.c_str(), 
		DifficultyName.length(), 
		SQLITE_STATIC
	));

	int r = sqlite3_step(st_GetDiffIDFile);

	if (IDOut)
	{
		if (r == SQLITE_ROW)
			*IDOut = sqlite3_column_int(st_GetDiffIDFile, 0);
		else
			*IDOut = 0;
	}

	SC(sqlite3_reset(st_GetDiffIDFile));
	return r == SQLITE_ROW;
}

void SongDatabase::UpdateDiffInternal(int &ret, int DiffID, Game::Song::Difficulty * Diff)
{
	SC(sqlite3_bind_int(
		st_DiffUpdateQuery,
		sqlite3_bind_parameter_index(st_DiffUpdateQuery, "$did"),
		DiffID));

	SC(sqlite3_bind_text(
		st_DiffUpdateQuery,
		sqlite3_bind_parameter_index(st_DiffUpdateQuery, "$name"),
		Diff->Name.c_str(),
		Diff->Name.length(),
		SQLITE_STATIC
	));

	SC(sqlite3_bind_double(
		st_DiffUpdateQuery,
		sqlite3_bind_parameter_index(st_DiffUpdateQuery, "$dur"),
		Diff->Duration
	));

	auto VDiff = static_cast<Game::VSRG::Difficulty*>(Diff);
	assert(VDiff->Data != NULL);

	SC(sqlite3_bind_int(
		st_DiffUpdateQuery,
		sqlite3_bind_parameter_index(st_DiffUpdateQuery, "$objcnt"),
		VDiff->Data->GetObjectCount()
	));

	SC(sqlite3_bind_int(
		st_DiffUpdateQuery,
		sqlite3_bind_parameter_index(st_DiffUpdateQuery, "$scoreobjcnt"),
		VDiff->Data->GetScoreItemsCount()
	));

	SC(sqlite3_bind_int(
		st_DiffUpdateQuery,
		sqlite3_bind_parameter_index(st_DiffUpdateQuery, "$virtual"),
		VDiff->IsVirtual
	));

	SC(sqlite3_bind_int(
		st_DiffUpdateQuery,
		sqlite3_bind_parameter_index(st_DiffUpdateQuery, "$keys"),
		VDiff->Channels
	));

	SC(sqlite3_bind_int64(
		st_DiffUpdateQuery,
		sqlite3_bind_parameter_index(st_DiffUpdateQuery, "$level"),
		VDiff->Level
	));

	SC(sqlite3_bind_text(
		st_DiffUpdateQuery,
		sqlite3_bind_parameter_index(st_DiffUpdateQuery, "$author"),
		VDiff->Author.c_str(), 
		VDiff->Author.length(), 
		SQLITE_STATIC
	));

	SC(sqlite3_bind_text(
		st_DiffUpdateQuery,
		sqlite3_bind_parameter_index(st_DiffUpdateQuery, "$stagefile"),
		VDiff->Data->StageFile.c_str(), 
		VDiff->Data->StageFile.length(),
		SQLITE_STATIC
	));

	SC(sqlite3_bind_text(
		st_DiffUpdateQuery,
		sqlite3_bind_parameter_index(st_DiffUpdateQuery, "$genre"),
		VDiff->Data->Genre.c_str(), 
		VDiff->Data->Genre.length(), 
		SQLITE_STATIC
	));


	SCS(sqlite3_step(st_DiffUpdateQuery));
	SC(sqlite3_reset(st_DiffUpdateQuery));
}

void SongDatabase::AssociateSong(Game::VSRG::Song *New)
{
	int ID;

	if (!New->Difficulties.size())
		return;

	// All difficulties have the same song ID, so..
	ID = GetSongIDForFile(New->Difficulties.at(0)->Filename);
	if (ID == -1) {
		ID = InsertSongInternal(New);
	}

	New->ID = ID;

	// Do the update, with the either new or old difficulty.
	for (auto k = New->Difficulties.begin();
		k != New->Difficulties.end();
		++k)
	{
		InsertOrUpdateDifficulty(ID, k->get());
		(*k)->Destroy();
	}
}

void SongDatabase::InsertOrUpdateDifficulty(int SongID, Game::VSRG::Difficulty* Diff)
{
	int FileID = InsertOrUpdateChartFile(Diff);
	int DiffID;
	int ret;

	if (!DifficultyExists(FileID, Diff->Name, &DiffID))
		InsertDiffInternal(ret, SongID, FileID, Diff);
	else // Update
		UpdateDiffInternal(ret, DiffID, Diff);

	if (DiffID == 0)
	{
		if (!DifficultyExists(FileID, Diff->Name, &DiffID))
			Utility::DebugBreak();
		if (DiffID == 0)
			Utility::DebugBreak();
	}

	Diff->ID = DiffID;
}

int SongDatabase::InsertSongInternal(Game::VSRG::Song * Song)
{
	int ret = 0;
	auto u8sfn = Utility::ToU8(Song->SongFilename.wstring());
	auto u8bfn = Utility::ToU8(Song->BackgroundFilename.wstring());
	auto u8pfn = Utility::ToU8(Song->SongPreviewSource.wstring());

	// Okay then, insert the song.
	// So now the latest entry is what we're going to insert difficulties and files into.
	SC(sqlite3_bind_text(st_SngInsertQuery,
		sqlite3_bind_parameter_index(st_SngInsertQuery, "$title"),
		Song->Title.c_str(), 
		Song->Title.length(), 
		SQLITE_STATIC
	));

	SC(sqlite3_bind_text(st_SngInsertQuery,
		sqlite3_bind_parameter_index(st_SngInsertQuery, "$author"),
		Song->Artist.c_str(), 
		Song->Artist.length(), 
		SQLITE_STATIC
	));

	SC(sqlite3_bind_text(
		st_SngInsertQuery,
		sqlite3_bind_parameter_index(st_SngInsertQuery, "$subtitle"),
		Song->Subtitle.c_str(), 
		Song->Subtitle.length(), 
		SQLITE_STATIC
	));

	SC(sqlite3_bind_text(
		st_SngInsertQuery,
		sqlite3_bind_parameter_index(st_SngInsertQuery, "$fn"),
		u8sfn.c_str(), 
		u8sfn.length(), 
		SQLITE_STATIC
	));

	SC(sqlite3_bind_text(
		st_SngInsertQuery,
		sqlite3_bind_parameter_index(st_SngInsertQuery, "$bg"),
		u8bfn.c_str(), 
		u8bfn.length(), 
		SQLITE_STATIC
	));

	SC(sqlite3_bind_text(
		st_SngInsertQuery,
		sqlite3_bind_parameter_index(st_SngInsertQuery, "$psong"),
		u8pfn.c_str(), 
		u8pfn.length(), 
		SQLITE_STATIC
	));

	SC(sqlite3_bind_double(
		st_SngInsertQuery,
		sqlite3_bind_parameter_index(st_SngInsertQuery, "$ptime"),
		Song->PreviewTime
	));

	SCS(sqlite3_step(st_SngInsertQuery));
	SC(sqlite3_reset(st_SngInsertQuery));

	sqlite3_step(st_GetLastSongID);
	int Out = sqlite3_column_int(st_GetLastSongID, 0);
	sqlite3_reset(st_GetLastSongID);

	return Out;
}

void SongDatabase::InsertDiffInternal(int &ret, int SongID, int FileID, Game::Song::Difficulty * Diff)
{
	SC(sqlite3_bind_int(
		st_DiffInsertQuery,
		sqlite3_bind_parameter_index(st_DiffInsertQuery, "$sid"),
		SongID
	));

	SC(sqlite3_bind_int(
		st_DiffInsertQuery,
		sqlite3_bind_parameter_index(st_DiffInsertQuery, "$fid"),
		FileID
	));

	SC(sqlite3_bind_text(
		st_DiffInsertQuery,
		sqlite3_bind_parameter_index(st_DiffInsertQuery, "$name"),
		Diff->Name.c_str(), 
		Diff->Name.length(),
		SQLITE_STATIC
	));

	SC(sqlite3_bind_double(
		st_DiffInsertQuery,
		sqlite3_bind_parameter_index(st_DiffInsertQuery, "$dur"), 
		Diff->Duration
	));

	SC(sqlite3_bind_text(
		st_DiffInsertQuery,
		sqlite3_bind_parameter_index(st_DiffInsertQuery, "$author"),
		Diff->Author.c_str(), 
		Diff->Author.length(), 
		SQLITE_STATIC
	));

	auto VDiff = static_cast<Game::VSRG::Difficulty*>(Diff);

	SC(sqlite3_bind_int(
		st_DiffInsertQuery,
		sqlite3_bind_parameter_index(st_DiffInsertQuery, "$objcnt"),
		VDiff->Data->GetObjectCount()
	));

	SC(sqlite3_bind_int(
		st_DiffInsertQuery,
		sqlite3_bind_parameter_index(st_DiffInsertQuery, "$scoreobjcnt"),
		VDiff->Data->GetScoreItemsCount()
	));

	SC(sqlite3_bind_int(
		st_DiffInsertQuery,
		sqlite3_bind_parameter_index(st_DiffInsertQuery, "$virtual"),
		VDiff->IsVirtual
	));

	SC(sqlite3_bind_int(
		st_DiffInsertQuery,
		sqlite3_bind_parameter_index(st_DiffInsertQuery, "$keys"),
		VDiff->Channels
	));

	SC(sqlite3_bind_int64(
		st_DiffInsertQuery,
		sqlite3_bind_parameter_index(st_DiffInsertQuery, "$level"),
		VDiff->Level
	));

	SC(sqlite3_bind_text(
		st_DiffInsertQuery,
		sqlite3_bind_parameter_index(st_DiffInsertQuery, "$genre"),
		VDiff->Data->Genre.c_str(), 
		VDiff->Data->Genre.length(), 
		SQLITE_STATIC
	));

	SC(sqlite3_bind_text(
		st_DiffInsertQuery,
		sqlite3_bind_parameter_index(st_DiffInsertQuery, "$stagefile"),
		VDiff->Data->StageFile.c_str(), 
		VDiff->Data->StageFile.length(), 
		SQLITE_STATIC
	));


	SCS(sqlite3_step(st_DiffInsertQuery));
	SC(sqlite3_reset(st_DiffInsertQuery));
}

std::filesystem::path SongDatabase::GetDifficultyFilename(int ID)
{
	int ret;
	SC(sqlite3_bind_int(st_GetDiffFilename, 1, ID));
	SCS(sqlite3_step(st_GetDiffFilename));

#ifdef _WIN32
	std::filesystem::path out = Utility::Widen((char*)sqlite3_column_text(st_GetDiffFilename, 0));
#else
	std::filesystem::path out = (char*)sqlite3_column_text(st_GetDiffFilename, 0);
#endif
	SC(sqlite3_reset(st_GetDiffFilename));
	return out;
}

std::filesystem::path SongDatabase::GetChartFilename(std::string hash)
{
	sqlite3_bind_text(
		st_FileFromHash,
		sqlite3_bind_parameter_index(st_FileFromHash, "$hash"),
		hash.c_str(), 
		hash.length(), 
		SQLITE_STATIC
	);

	auto res = sqlite3_step(st_FileFromHash);

	auto ret = std::filesystem::path();

	// if there's more than one candidate file, 
	// find the first one that exists
	// ret will be last declared if none exist
	while (res == SQLITE_ROW) {
		char* out = (char*)sqlite3_column_text(st_FileFromHash, 0);
		ret = out ? out : "";

		if (std::filesystem::exists(ret))
			break;
		else
			sqlite3_step(st_FileFromHash);
	}

	sqlite3_reset(st_FileFromHash);

	return ret;
}

std::string SongDatabase::GetChartHash(std::filesystem::path filename)
{
	std::string u8p = Utility::ToU8(std::filesystem::absolute(filename).wstring());

	sqlite3_bind_text(
		st_HashFromFile,
		sqlite3_bind_parameter_index(st_HashFromFile, "$filename"),
		u8p.c_str(), 
		u8p.length(), 
		SQLITE_STATIC
	);

	auto res = sqlite3_step(st_HashFromFile);

	auto ret = std::string();

	if (res == SQLITE_ROW) {
		auto out = (char*)sqlite3_column_text(st_HashFromFile, 0);
		ret = out;
	}
	else {
		ret = Utility::GetSha256ForFile(filename);
	}

	sqlite3_reset(st_HashFromFile);

	return ret;
}

bool SongDatabase::CacheNeedsRenewal(std::filesystem::path Dir)
{
	// must match what we put at InsertFilename time, so turn into absolute path on both places!
	auto u8p = std::filesystem::absolute(Dir).u8string();
	int CurLMT = Utility::GetLastModifiedTime(Dir);
	bool NeedsRenewal;
	int res, ret;

	SC(sqlite3_bind_text(
		stGetLMTQuery,
		sqlite3_bind_parameter_index(stGetLMTQuery, "$fn"),
		u8p.c_str(), 
		u8p.length(), 
		SQLITE_STATIC
	));

	res = sqlite3_step(stGetLMTQuery);

	if (res == SQLITE_ROW) // entry exists
	{
		int OldLMT = sqlite3_column_int(stGetLMTQuery, 0);
		bool IsLMTCurrent = (CurLMT == OldLMT); // file was not modified since last time
		NeedsRenewal = !IsLMTCurrent;
	}
	else
	{
		NeedsRenewal = true;
	}

	SC(sqlite3_reset(stGetLMTQuery));
	return NeedsRenewal;
}

void SongDatabase::StartTransaction()
{
	char* tail;
	sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, &tail);
}

void SongDatabase::EndTransaction()
{
	char* tail;
	sqlite3_exec(db, "COMMIT;", NULL, NULL, &tail);
}

std::string SongDatabase::GetArtistForDifficulty(int ID)
{
	int rs;
	std::string out;

	sqlite3_bind_int(
		st_GetDiffAuthor,
		sqlite3_bind_parameter_index(st_GetDiffAuthor, "$did"),
		ID
	);

	rs = sqlite3_step(st_GetDiffAuthor);

	if (rs == SQLITE_ROW)
		out = (char*)sqlite3_column_text(st_GetDiffAuthor, 0);

	sqlite3_reset(st_GetDiffAuthor);
	return out;
}

std::string SongDatabase::GetGenreForDifficulty(int DiffID)
{
	std::string out;

	sqlite3_bind_int(st_GetDiffGenre,
		sqlite3_bind_parameter_index(st_GetDiffGenre, "$did"),
		DiffID);

	if (sqlite3_step(st_GetDiffGenre) == SQLITE_ROW)
		out = (char*)sqlite3_column_text(st_GetDiffGenre, 0);

	sqlite3_reset(st_GetDiffGenre);
	return out;
}

#ifdef _WIN32
#define _W(x) Utility::Widen((char*)x)
#else
#define _W(x) ((char*)x)
#endif

void SongDatabase::GetSongInformation(int ID, Game::VSRG::Song* Out)
{
	int ret;

	SC(sqlite3_bind_int(
		stGetSongInfo,
		sqlite3_bind_parameter_index(stGetSongInfo, "$sid"),
		ID
	));

	ret = sqlite3_step(stGetSongInfo);

	if (ret != SQLITE_ROW)
	{
		Log::Printf("SongDatabase::GetSongInformation: Song %d does not exist.\n", ID);
		return;
	}

	// Main metadata is up in this query.

	// oh god there is no better way without keeping track of column names for a
	// statement by yourself...
	Out->Title = (char*)sqlite3_column_text(stGetSongInfo, 0);
	Out->Artist = (char*)sqlite3_column_text(stGetSongInfo, 1);
	Out->SongFilename = _W(sqlite3_column_text(stGetSongInfo, 2));
	Out->Subtitle = (char*)sqlite3_column_text(stGetSongInfo, 3);
	// Out->BackgroundFilename = _W(sqlite3_column_text(stGetSongInfo, 4));
	Out->ID = ID;
	Out->PreviewTime = sqlite3_column_double(stGetSongInfo, 5);

	SC(sqlite3_reset(stGetSongInfo));

	// Now, difficulty information.
	SC(sqlite3_bind_int(stGetDiffInfo,
		sqlite3_bind_parameter_index(stGetDiffInfo, "$sid"),
		ID));

	while (sqlite3_step(stGetDiffInfo) != SQLITE_DONE)
	{
		auto Diff = std::make_shared<Game::VSRG::Difficulty>();

		// diffid associated data
		Diff->ID = sqlite3_column_int(stGetDiffInfo, 0);
		Diff->Name = (char*)sqlite3_column_text(stGetDiffInfo, 1);
		Diff->Duration = sqlite3_column_double(stGetDiffInfo, 4);
		Diff->IsVirtual = (sqlite3_column_int(stGetDiffInfo, 5) == 1);
		Diff->Channels = sqlite3_column_int(stGetDiffInfo, 6);

		// We don't include author information to force querying it from the database.
		// Diff->Author
		Diff->Level = sqlite3_column_int64(stGetDiffInfo, 8);

		// File ID associated data
		int FileID = sqlite3_column_int(stGetDiffInfo, 7);

		SC(sqlite3_bind_int(
			st_GetFileInfo,
			sqlite3_bind_parameter_index(st_GetFileInfo, "$fid"),
			FileID
		));

		sqlite3_step(st_GetFileInfo);

		// Widen can throw, so we should reset the info _before_ we try to copy.
		std::string s = (char*)sqlite3_column_text(st_GetFileInfo, 0);

		// There's a case where a string could cause operator= to throw
		// if it tries encoding from u8 into the internal ::path representation and it fails!
		try {
#ifdef _WIN32
			Diff->Filename = Utility::Widen(s);
#else
			Diff->Filename = s;
#endif
		}
		catch (std::exception &e) {
			// We failed copying this thing - clean up and rethrow.
			SC(sqlite3_reset(st_GetFileInfo));
			SC(sqlite3_reset(stGetDiffInfo));
			throw e;
		}

		SC(sqlite3_reset(st_GetFileInfo));
		Out->Difficulties.push_back(Diff);
	}

	SC(sqlite3_reset(stGetDiffInfo));
}

int SongDatabase::GetSongIDForFile(std::filesystem::path File)
{
	int ret;
	int Out = -1;
	const std::string u8path = std::filesystem::absolute(File).u8string();

	SC(sqlite3_bind_text(
		st_GetSIDFromFilename,
		sqlite3_bind_parameter_index(st_GetSIDFromFilename, "$fn"),
		u8path.c_str(), 
		u8path.length(), 
		SQLITE_STATIC
	));

	int r = sqlite3_step(st_GetSIDFromFilename);
	if (r == SQLITE_ROW)
	{
		// We found a song with ID and everything..
		Out = sqlite3_column_int(st_GetSIDFromFilename, 0);
	}

	sqlite3_reset(st_GetSIDFromFilename);
	return Out;
}

std::string SongDatabase::GetStageFile(int DiffID)
{
	int ret;

	SC(sqlite3_bind_int(
		st_GetStageFile,
		sqlite3_bind_parameter_index(st_GetStageFile, "$did"),
		DiffID
	));

	SCS(sqlite3_step(st_GetStageFile));

	const char* sOut = (const char*)sqlite3_column_text(st_GetStageFile, 0);
	std::string Out = sOut ? sOut : "";

	SC(sqlite3_reset(st_GetStageFile));
	return Out;
}

void SongDatabase::GetPreviewInfo(int SongID, std::string &Filename, float &PreviewStart)
{
	int ret;
	SC(sqlite3_bind_int(
		st_GetPreviewInfo,
		sqlite3_bind_parameter_index(st_GetPreviewInfo, "$sid"),
		SongID
	));

	SCS(sqlite3_step(st_GetPreviewInfo));

	const char* sOut = (const char*)sqlite3_column_text(st_GetPreviewInfo, 0);
	float fOut = sqlite3_column_double(st_GetPreviewInfo, 1);

	std::string Out = sOut ? sOut : "";

	SC(sqlite3_reset(st_GetPreviewInfo));
	Filename = Out;
	PreviewStart = fOut;
}
