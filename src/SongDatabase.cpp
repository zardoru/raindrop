#include "GameGlobal.h"
#include "Logging.h"

#include "Song7K.h"
#include "SongDatabase.h"

#include <sqlite3.h>

char *DatabaseQuery =
  "CREATE TABLE IF NOT EXISTS [songdb] (\
  [id] INTEGER, \
  [directory] varchar(260), \
  [songtitle] varchar(260), \
  [songauthor] varchar(260), \
  [songfilename] varchar(260), \
  [songbackground] varchar(260), \
  [mode] INT, \
  PRIMARY KEY ([id]));\
CREATE TABLE IF NOT EXISTS [songfiledb] (\
  [id] INTEGER PRIMARY KEY, \
  [filename] varchar(260), \
  [lastmodified] INTEGER);\
CREATE TABLE IF NOT EXISTS [diffdb] (\
  [songid] INTEGER CONSTRAINT [sid] REFERENCES [songdb]([id]) ON DELETE CASCADE, \
  [diffid] INTEGER PRIMARY KEY, \
  [fileid] INT CONSTRAINT [fid] REFERENCES [songfiledb]([id]) ON DELETE CASCADE, \
  [name] VARCHAR(260), \
  [objcount] INT, \
  [scoreobjectcount] INT, \
  [holdcount] INT, \
  [notecount] INT, \
  [duration] DOUBLE, \
  [isvirtual] INTEGER, \
  [keys] INTEGER,\
  [bpmtype] INT);";

const char* GetSongIDQuery = "SELECT * FROM songdb WHERE directory=?";
const char* InsertSongQuery = "INSERT INTO songdb VALUES (NULL,?,?,?,?,?,?)";
const char* InsertDifficultyQuery = "INSERT INTO diffdb VALUES (?,NULL,?,?,?,?,?,?,?,?,?,?)";
const char* GetFilenameIDQuery = "SELECT id FROM songfiledb WHERE filename=? AND lastmodified=?";
const char* InsertFilenameQuery = "INSERT INTO songfiledb VALUES (NULL,?,?)";
const char* GetDiffIDQuery = "SELECT diffid FROM diffdb \
							 WHERE (diffdb.fileid = (SELECT songfiledb.id FROM songfiledb WHERE filename=?)) AND\
							 diffdb.name = ?";
const char* GetDiffNameQuery = "SELECT name FROM diffdb \
							 WHERE (diffdb.fileid = (SELECT songfiledb.id FROM songfiledb WHERE filename=?))";
const char* GetLMTQuery = "SELECT lastmodified FROM songfiledb WHERE filename=?";
const char* RenewDiff = "DELETE FROM diffdb WHERE songid=?";
const char* GetSongInfo = "SELECT songtitle, songauthor, songfilename, songbackground, mode FROM songdb WHERE id=?";
const char* GetDiffInfo = "SELECT diffid, name, objcount, scoreobjectcount, holdcount, notecount, duration, isvirtual, keys, fileid, bpmtype FROM diffdb WHERE songid=?";
const char* GetFileInfo = "SELECT filename, lastmodified FROM songfiledb WHERE id=?";
const char* UpdateLMT = "UPDATE songfiledb SET lastmodified=? WHERE id=?";
const char* UpdateDiff = "UPDATE diffdb SET name=?,objcount=?,scoreobjectcount=?,holdcount=?,notecount=?,\
	duration=?,isvirtual=?,keys=?,bpmtype=? WHERE diffid=?";

const char* GetDiffFilename = "SELECT filename FROM songfiledb WHERE (songfiledb.id = (SELECT diffdb.fileid FROM diffdb WHERE diffid=?))";

const char* GetDiffIDFileID = "SELECT diffid FROM diffdb \
							 WHERE diffdb.fileid=? AND\
							 							 diffdb.name = ?";

#define SC(x) ret=x; if(ret!=SQLITE_OK) {Log::Printf("sqlite: %ls (code %d)\n",Utility::Widen(sqlite3_errmsg(db)).c_str(), ret); Utility::DebugBreak(); }
#define SCS(x) ret=x; if(ret!=SQLITE_DONE) {Log::Printf("sqlite: %ls (code %d)\n",Utility::Widen(sqlite3_errmsg(db)).c_str(), ret); Utility::DebugBreak(); }
SongDatabase::SongDatabase(String Database)
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
		SC(sqlite3_prepare_v2(db, GetSongIDQuery, strlen(GetSongIDQuery), &st_IDQuery, &tail));
		SC(sqlite3_prepare_v2(db, InsertSongQuery, strlen(InsertSongQuery), &st_SngInsertQuery, &tail));
		SC(sqlite3_prepare_v2(db, InsertDifficultyQuery, strlen(InsertDifficultyQuery), &st_DiffInsertQuery, &tail));
		SC(sqlite3_prepare_v2(db, GetFilenameIDQuery, strlen(GetFilenameIDQuery), &st_FilenameQuery, &tail));
		SC(sqlite3_prepare_v2(db, InsertFilenameQuery, strlen(InsertFilenameQuery), &st_FilenameInsertQuery, &tail));
		SC(sqlite3_prepare_v2(db, GetDiffIDQuery, strlen(GetDiffIDQuery), &st_DiffIDQuery, &tail));
		SC(sqlite3_prepare_v2(db, GetLMTQuery, strlen(GetLMTQuery), &st_LMTQuery, &tail));
		SC(sqlite3_prepare_v2(db, RenewDiff, strlen(RenewDiff), &st_DelDiffsQuery, &tail));
		SC(sqlite3_prepare_v2(db, GetSongInfo, strlen(GetSongInfo), &st_GetSongInfo, &tail));
		SC(sqlite3_prepare_v2(db, GetDiffInfo, strlen(GetDiffInfo), &st_GetDiffInfo, &tail));
		SC(sqlite3_prepare_v2(db, GetFileInfo, strlen(GetFileInfo), &st_GetFileInfo, &tail));
		SC(sqlite3_prepare_v2(db, UpdateLMT, strlen(UpdateLMT), &st_UpdateLMT, &tail));
		SC(sqlite3_prepare_v2(db, GetDiffNameQuery, strlen(GetDiffNameQuery), &st_GetDiffNameQuery, &tail));
		SC(sqlite3_prepare_v2(db, GetDiffIDFileID, strlen(GetDiffIDFileID), &st_GetDiffIDFile, &tail));
		SC(sqlite3_prepare_v2(db, UpdateDiff, strlen(UpdateDiff), &st_DiffUpdateQuery, &tail));
		SC(sqlite3_prepare_v2(db, GetDiffFilename, strlen(GetDiffFilename), &st_GetDiffFilename, &tail));
	}
}

SongDatabase::~SongDatabase()
{
	if (db)
	{
		sqlite3_finalize(st_IDQuery);
		sqlite3_finalize(st_SngInsertQuery);
		sqlite3_finalize(st_DiffInsertQuery);
		sqlite3_finalize(st_FilenameQuery);
		sqlite3_finalize(st_FilenameInsertQuery);
		sqlite3_finalize(st_DiffIDQuery);
		sqlite3_finalize(st_LMTQuery);
		sqlite3_finalize(st_DelDiffsQuery);
		sqlite3_finalize(st_GetSongInfo);
		sqlite3_finalize(st_GetDiffInfo);
		sqlite3_finalize(st_UpdateLMT);
		sqlite3_finalize(st_GetDiffNameQuery);
		sqlite3_finalize(st_GetDiffIDFile);
		sqlite3_finalize(st_DiffUpdateQuery);
		sqlite3_finalize(st_GetDiffFilename);
		sqlite3_close(db);
	}
}

// Returns: Song ID.
int SongDatabase::AddSong(Directory Dir, int Mode, Game::Song* In)
{
	int ID;
	int ret;

	if (!IsSongDirectory(Dir, &ID)) // Song does not exist in the database, database, woah woah
	{
		SC(sqlite3_bind_text(st_SngInsertQuery, 1, Dir.c_path(), Dir.path().length(), SQLITE_TRANSIENT));
		SC(sqlite3_bind_text(st_SngInsertQuery, 2, In->SongName.c_str(), In->SongName.length(), SQLITE_TRANSIENT));
		SC(sqlite3_bind_text(st_SngInsertQuery, 3, In->SongAuthor.c_str(), In->SongAuthor.length(), SQLITE_TRANSIENT));
		SC(sqlite3_bind_text(st_SngInsertQuery, 4, In->SongFilename.c_str(), In->SongFilename.length(), SQLITE_TRANSIENT));
		SC(sqlite3_bind_text(st_SngInsertQuery, 5, In->BackgroundFilename.c_str(), In->BackgroundFilename.length(), SQLITE_TRANSIENT));
		SC(sqlite3_bind_int(st_SngInsertQuery, 6, Mode));
		SCS(sqlite3_step(st_SngInsertQuery));
		SC(sqlite3_reset(st_SngInsertQuery));
	}

	// Call again to get the actual ID!
	IsSongDirectory(Dir, &ID);

	return ID;
}

// Only returns whether the directory exists in the database.
// ID of the song if it exists.
bool SongDatabase::IsSongDirectory(Directory Dir, int *IDOut)
{
	int ret;
	SC(sqlite3_bind_text(st_IDQuery, 1, Dir.c_path(), Dir.path().length(), SQLITE_TRANSIENT));

	ret = sqlite3_step(st_IDQuery);

	if (ret == SQLITE_ROW)
	{
		if (IDOut)
			*IDOut = sqlite3_column_int(st_IDQuery, 0);
	}

	sqlite3_reset(st_IDQuery);
	return ret == SQLITE_ROW;
}

// Inserts a filename, if it already exists, updates it.
// Returns the ID of the filename.
int SongDatabase::InsertFilename(Directory Fn)
{
	int ret;
	int idOut;
	SC(sqlite3_bind_text(st_FilenameQuery, 1, Fn.c_path(), Fn.path().length(), SQLITE_TRANSIENT));
	SC(sqlite3_bind_int(st_FilenameQuery, 2, Utility::GetLMT(Fn.path())));

	if (sqlite3_step(st_FilenameQuery) == SQLITE_ROW)
	{
		// There's an entry. Move along.
		idOut = sqlite3_column_int(st_FilenameQuery, 0);

		// Update the last-modified-time of this file.
		SC(sqlite3_bind_int(st_UpdateLMT, 1, Utility::GetLMT(Fn.path())));
		SC(sqlite3_bind_int(st_UpdateLMT, 2, idOut));
		SCS(sqlite3_step(st_UpdateLMT));
		SC(sqlite3_reset(st_UpdateLMT));
	}else
	{
		// There's no entry, got to insert it.
		SC(sqlite3_bind_text(st_FilenameInsertQuery, 1, Fn.c_path(), Fn.path().length(), SQLITE_TRANSIENT));
		SC(sqlite3_bind_int(st_FilenameInsertQuery, 2, Utility::GetLMT(Fn.path())));
		SCS(sqlite3_step(st_FilenameInsertQuery)); // This should not fail. Otherwise, there are bigger problems to worry about...
		SC(sqlite3_reset(st_FilenameInsertQuery));

		// okay, then return the ID.
		SC(sqlite3_reset(st_FilenameQuery));
		SC(sqlite3_bind_text(st_FilenameQuery, 1, Fn.c_path(), Fn.path().length(), SQLITE_TRANSIENT));
		SC(sqlite3_bind_int(st_FilenameQuery, 2, Utility::GetLMT(Fn.path())));
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
bool SongDatabase::DifficultyExists(int FileID, String DifficultyName, int *IDOut)
{
	int ret;
	SC(sqlite3_bind_int (st_GetDiffIDFile, 1, FileID));
	SC(sqlite3_bind_text(st_GetDiffIDFile, 2, DifficultyName.c_str(), DifficultyName.length(), SQLITE_TRANSIENT));
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

// Adds a difficulty to the database, or updates it if it already exists.
void SongDatabase::AddDifficulty(int SongID, Directory Filename, Game::Song::Difficulty* Diff, int Mode)
{
	int FileID = InsertFilename(Filename);
	int DiffID;
	int ret;

	if (!DifficultyExists(FileID, Diff->Name, &DiffID))
	{
		SC(sqlite3_bind_int(st_DiffInsertQuery, 1, SongID));
		SC(sqlite3_bind_int(st_DiffInsertQuery, 2, FileID));
		SC(sqlite3_bind_text(st_DiffInsertQuery, 3, Diff->Name.c_str(), Diff->Name.length(), SQLITE_TRANSIENT));
		SC(sqlite3_bind_int(st_DiffInsertQuery, 4, Diff->TotalObjects));
		SC(sqlite3_bind_int(st_DiffInsertQuery, 5, Diff->TotalScoringObjects));
		SC(sqlite3_bind_int(st_DiffInsertQuery, 6, Diff->TotalHolds));
		SC(sqlite3_bind_int(st_DiffInsertQuery, 7, Diff->TotalNotes));
		SC(sqlite3_bind_double(st_DiffInsertQuery, 8, Diff->Duration));

		if (Mode == MODE_7K)
		{
			VSRG::Difficulty *VDiff = static_cast<VSRG::Difficulty*>(Diff);

			SC(sqlite3_bind_int(st_DiffInsertQuery, 9, VDiff->IsVirtual));
			SC(sqlite3_bind_int(st_DiffInsertQuery, 10, VDiff->Channels));
			SC(sqlite3_bind_int(st_DiffInsertQuery, 11, VDiff->BPMType));
		}else if (Mode == MODE_DOTCUR)
		{
			SC(sqlite3_bind_int(st_DiffInsertQuery, 9, 0));
			SC(sqlite3_bind_int(st_DiffInsertQuery, 10, 0));
			SC(sqlite3_bind_int(st_DiffInsertQuery, 11, 0));
		}

		SCS(sqlite3_step(st_DiffInsertQuery));
		SC(sqlite3_reset(st_DiffInsertQuery));

	}else
	{
		SC(sqlite3_bind_text(st_DiffUpdateQuery, 1, Diff->Name.c_str(), Diff->Name.length(), SQLITE_TRANSIENT));
		SC(sqlite3_bind_int(st_DiffUpdateQuery, 2, Diff->TotalObjects));
		SC(sqlite3_bind_int(st_DiffUpdateQuery, 3, Diff->TotalScoringObjects));
		SC(sqlite3_bind_int(st_DiffUpdateQuery, 4, Diff->TotalHolds));
		SC(sqlite3_bind_int(st_DiffUpdateQuery, 5, Diff->TotalNotes));
		SC(sqlite3_bind_double(st_DiffUpdateQuery, 6, Diff->Duration));

		if (Mode == MODE_7K)
		{
			VSRG::Difficulty *VDiff = static_cast<VSRG::Difficulty*>(Diff);

			SC(sqlite3_bind_int(st_DiffUpdateQuery, 7, VDiff->IsVirtual));
			SC(sqlite3_bind_int(st_DiffUpdateQuery, 8, VDiff->Channels));
			SC(sqlite3_bind_int(st_DiffUpdateQuery, 9, VDiff->BPMType));
		}else if (Mode == MODE_DOTCUR)
		{
			SC(sqlite3_bind_int(st_DiffUpdateQuery, 7, 0));
			SC(sqlite3_bind_int(st_DiffUpdateQuery, 8, 0));
			SC(sqlite3_bind_int(st_DiffUpdateQuery, 9, 0));
		}

		SC(sqlite3_bind_int(st_DiffUpdateQuery, 10, DiffID));
		SCS(sqlite3_step(st_DiffUpdateQuery));
		SC(sqlite3_reset(st_DiffUpdateQuery));
	}

	if (DiffID == 0)
	{
		if (!DifficultyExists (FileID, Diff->Name, &DiffID))
			Utility::DebugBreak();
		if(DiffID == 0)
			Utility::DebugBreak();
	}

	Diff->ID = DiffID;
}

String SongDatabase::GetDifficultyFilename (int ID)
{
	int ret;
	SC(sqlite3_bind_int(st_GetDiffFilename, 1, ID));
	SCS(sqlite3_step(st_GetDiffFilename));

	String out = (char *)sqlite3_column_text(st_GetDiffFilename, 0);
	SC(sqlite3_reset(st_GetDiffFilename));
	return out;
}

bool SongDatabase::CacheNeedsRenewal(Directory Dir)
{
	int CurLMT = Utility::GetLMT(Dir.path());
	bool NeedsRenewal;
	int res, ret;

	SC(sqlite3_bind_text(st_LMTQuery, 1, Dir.c_path(), Dir.path().length(), SQLITE_TRANSIENT));
	res = sqlite3_step(st_LMTQuery);

	if (res == SQLITE_ROW) // entry exists
	{
		int OldLMT = sqlite3_column_int(st_LMTQuery, 0);
		bool IsLMTCurrent = (CurLMT==OldLMT); // file was not modified since last time
		NeedsRenewal = !IsLMTCurrent;
	}else {
		NeedsRenewal = true;
	}

	SC(sqlite3_reset(st_LMTQuery));
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

void SongDatabase::GetSongInformation7K (int ID, VSRG::Song* Out)
{
	int ret;
	SC(sqlite3_bind_int(st_GetSongInfo, 1, ID));
	ret = sqlite3_step(st_GetSongInfo);

	if (ret != SQLITE_ROW)
	{
		Log::Printf("SongDatabase::GetSongInformation7K: Chart %d does not exist.\n", ID);
		return;
	}

	// Main metadata is up in this query.

	Out->SongName = (char*)sqlite3_column_text(st_GetSongInfo, 0);
	Out->SongAuthor = (char*)sqlite3_column_text(st_GetSongInfo, 1);
	Out->SongFilename = (char*)sqlite3_column_text(st_GetSongInfo, 2);
	Out->BackgroundFilename = (char*)sqlite3_column_text(st_GetSongInfo, 3);
	int mode = sqlite3_column_int(st_GetSongInfo, 4);

	SC(sqlite3_reset(st_GetSongInfo));

	if (mode != MODE_7K)
		return; // Sowwy.

	// Now, difficulty information.
	SC(sqlite3_bind_int(st_GetDiffInfo, 1, ID));
	while (sqlite3_step(st_GetDiffInfo) != SQLITE_DONE)
	{
		VSRG::Difficulty * Diff = new VSRG::Difficulty;

		// diffid associated data
		Diff->ID = sqlite3_column_int(st_GetDiffInfo, 0);
		Diff->Name = (char*)sqlite3_column_text(st_GetDiffInfo, 1);
		Diff->TotalObjects = sqlite3_column_int(st_GetDiffInfo, 2);
		Diff->TotalScoringObjects = sqlite3_column_int(st_GetDiffInfo, 3);
		Diff->TotalHolds = sqlite3_column_int(st_GetDiffInfo, 4);
		Diff->TotalNotes = sqlite3_column_int(st_GetDiffInfo, 5);
		Diff->Duration = sqlite3_column_double(st_GetDiffInfo, 6);
		Diff->IsVirtual = (sqlite3_column_int(st_GetDiffInfo, 7) == 1);
		Diff->Channels = sqlite3_column_int(st_GetDiffInfo, 8);

		int colInt = sqlite3_column_int(st_GetDiffInfo, 10);
		Diff->BPMType = (VSRG::Difficulty::EBt)colInt;


		// File ID associated data
		int FileID = sqlite3_column_int(st_GetDiffInfo, 9);

		SC(sqlite3_bind_int(st_GetFileInfo, 1, FileID));
		sqlite3_step(st_GetFileInfo);
		Diff->Filename = (char*)sqlite3_column_text(st_GetFileInfo, 0);
		Diff->LMT = sqlite3_column_int(st_GetFileInfo, 1);
		SC(sqlite3_reset(st_GetFileInfo));

		Out->Difficulties.push_back(Diff);
	}

	SC(sqlite3_reset(st_GetDiffInfo));
}
