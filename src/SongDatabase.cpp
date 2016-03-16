#include "pch.h"

#include "GameGlobal.h"
#include "Logging.h"

#include "Song7K.h"
#include "SongDatabase.h"

char *DatabaseQuery =
"CREATE TABLE IF NOT EXISTS [songdb] (\
  [id] INTEGER, \
  [songtitle] varchar(260), \
  [songauthor] varchar(260), \
  [subtitle] varchar(260), \
  [songfilename] varchar(260), \
  [songbackground] varchar(260), \
  [mode] INT, \
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
  [holdcount] INT, \
  [notecount] INT, \
  [duration] DOUBLE, \
  [isvirtual] INTEGER, \
  [keys] INTEGER,\
  [bpmtype] INT,\
  [level] INT,\
  [author] VARCHAR(256),\
  [stagefile] varchar(260));\
    CREATE INDEX IF NOT EXISTS song_index ON songfiledb(filename);\
	  CREATE INDEX IF NOT EXISTS diff_index ON diffdb(diffid, songid, fileid);\
	  CREATE INDEX IF NOT EXISTS songid_index ON songdb(id);\
  ";

const char* InsertSongQuery = "INSERT INTO songdb VALUES (NULL,?,?,?,?,?,?,?,?)";
const char* InsertDifficultyQuery = "INSERT INTO diffdb VALUES (?,NULL,?,?,?,?,?,?,?,?,?,?,?,?,?)";
const char* GetFilenameIDQuery = "SELECT id, lastmodified FROM songfiledb WHERE filename=?";
const char* InsertFilenameQuery = "INSERT INTO songfiledb VALUES (NULL,?,?,?)";
const char* GetDiffNameQuery = "SELECT name FROM diffdb \
							 WHERE (diffdb.fileid = (SELECT songfiledb.id FROM songfiledb WHERE filename=?))";
const char* GetLMTQuery = "SELECT lastmodified FROM songfiledb WHERE filename=?";
const char* GetSongInfo = "SELECT songtitle, songauthor, songfilename, subtitle, songbackground, mode, previewtime FROM songdb WHERE id=?";
const char* GetDiffInfo = "SELECT diffid, name, objcount, scoreobjectcount, holdcount, notecount, duration, isvirtual, \
						  keys, fileid, bpmtype, level FROM diffdb WHERE songid=?";
const char* GetFileInfo = "SELECT filename, lastmodified FROM songfiledb WHERE id=?";
const char* UpdateLMT = "UPDATE songfiledb SET lastmodified=?, hash=? WHERE filename=?";
const char* UpdateDiff = "UPDATE diffdb SET name=?,objcount=?,scoreobjectcount=?,holdcount=?,notecount=?,\
	duration=?,isvirtual=?,keys=?,bpmtype=?,level=?,author=?,stagefile=? WHERE diffid=?";

const char* GetDiffFilename = "SELECT filename FROM songfiledb WHERE (songfiledb.id = (SELECT diffdb.fileid FROM diffdb WHERE diffid=?))";

const char* GetDiffIDFileID = "SELECT diffid FROM diffdb \
							 WHERE diffdb.fileid=? AND\
							 							 diffdb.name = ?";

const char* GetSongIDFromFilename = "SELECT songid FROM diffdb WHERE (diffdb.fileid = (SELECT id FROM songfiledb WHERE filename=?))";
const char* GetLatestSongID = "SELECT MAX(id) FROM songdb";
const char* GetAuthorOfDifficulty = "SELECT author FROM diffdb WHERE diffid=?";
const char* GetPreviewOfSong = "SELECT previewsong, previewtime FROM songdb WHERE id=?";
const char* sGetStageFile = "SELECT stagefile FROM diffdb WHERE diffid=?";

#define SC(x) ret=x; if(ret!=SQLITE_OK && ret != SQLITE_DONE) {Log::Printf("sqlite: %ls (code %d)\n",Utility::Widen(sqlite3_errmsg(db)).c_str(), ret); Utility::DebugBreak(); }
#define SCS(x) ret=x; if(ret!=SQLITE_DONE && ret != SQLITE_ROW) {Log::Printf("sqlite: %ls (code %d)\n",Utility::Widen(sqlite3_errmsg(db)).c_str(), ret); Utility::DebugBreak(); }

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
        SC(sqlite3_prepare_v2(db, GetLMTQuery, strlen(GetLMTQuery), &st_LMTQuery, &tail));
        SC(sqlite3_prepare_v2(db, GetSongInfo, strlen(GetSongInfo), &st_GetSongInfo, &tail));
        SC(sqlite3_prepare_v2(db, GetDiffInfo, strlen(GetDiffInfo), &st_GetDiffInfo, &tail));
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
        sqlite3_finalize(st_LMTQuery);
        sqlite3_finalize(st_GetSongInfo);
        sqlite3_finalize(st_GetDiffInfo);
        sqlite3_finalize(st_UpdateLMT);
        sqlite3_finalize(st_GetDiffIDFile);
        sqlite3_finalize(st_DiffUpdateQuery);
        sqlite3_finalize(st_GetDiffFilename);
        sqlite3_finalize(st_GetSIDFromFilename);
        sqlite3_finalize(st_GetLastSongID);
        sqlite3_finalize(st_GetDiffAuthor);
        sqlite3_close(db);
    }
}

// Inserts a filename, if it already exists, updates it.
// Returns the ID of the filename.
int SongDatabase::InsertFilename(std::filesystem::path Fn)
{
    int ret;
    int idOut;
    int lmt;
	std::string u8p = Utility::Narrow(std::filesystem::absolute(Fn).wstring());

    SC(sqlite3_bind_text(st_FilenameQuery, 1, u8p.c_str(), u8p.length(), SQLITE_STATIC));

    if (sqlite3_step(st_FilenameQuery) == SQLITE_ROW)
    {
        idOut = sqlite3_column_int(st_FilenameQuery, 0);
        lmt = sqlite3_column_int(st_FilenameQuery, 1);

        int lastLmt = Utility::GetLMT(Fn);

        // Update the last-modified-time of this file, and its hash if it has changed.
        if (lmt != lastLmt)
        {
            std::string Hash = Utility::GetSha256ForFile(Fn);
            SC(sqlite3_bind_int(st_UpdateLMT, 1, lastLmt));
            SC(sqlite3_bind_text(st_UpdateLMT, 2, Hash.c_str(), Hash.length(), SQLITE_STATIC));
            SC(sqlite3_bind_text(st_UpdateLMT, 3, u8p.c_str(), u8p.length(), SQLITE_STATIC));
            SCS(sqlite3_step(st_UpdateLMT));
            SC(sqlite3_reset(st_UpdateLMT));
        }
    }
    else
    {
        std::string Hash = Utility::GetSha256ForFile(Fn);

        // There's no entry, got to insert it.
        SC(sqlite3_bind_text(st_FilenameInsertQuery, 1, u8p.c_str(), u8p.length(), SQLITE_STATIC));
        SC(sqlite3_bind_int(st_FilenameInsertQuery, 2, Utility::GetLMT(Fn)));
        SC(sqlite3_bind_text(st_FilenameInsertQuery, 3, Hash.c_str(), Hash.length(), SQLITE_STATIC));
        SCS(sqlite3_step(st_FilenameInsertQuery)); // This should not fail. Otherwise, there are bigger problems to worry about...
        SC(sqlite3_reset(st_FilenameInsertQuery));

        // okay, then return the ID.
        SC(sqlite3_reset(st_FilenameQuery));
        SC(sqlite3_bind_text(st_FilenameQuery, 1, u8p.c_str(), u8p.length(), SQLITE_STATIC));
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
    SC(sqlite3_bind_int(st_GetDiffIDFile, 1, FileID));
    SC(sqlite3_bind_text(st_GetDiffIDFile, 2, DifficultyName.c_str(), DifficultyName.length(), SQLITE_STATIC));
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
void SongDatabase::AddDifficulty(int SongID, std::filesystem::path Filename, Game::Song::Difficulty* Diff, int Mode)
{
    int FileID = InsertFilename(Filename);
    int DiffID;
    int ret;

    if (!DifficultyExists(FileID, Diff->Name, &DiffID))
    {
        SC(sqlite3_bind_int(st_DiffInsertQuery, 1, SongID));
        SC(sqlite3_bind_int(st_DiffInsertQuery, 2, FileID));
        SC(sqlite3_bind_text(st_DiffInsertQuery, 3, Diff->Name.c_str(), Diff->Name.length(), SQLITE_STATIC));
        SC(sqlite3_bind_int(st_DiffInsertQuery, 4, Diff->TotalObjects));
        SC(sqlite3_bind_int(st_DiffInsertQuery, 5, Diff->TotalScoringObjects));
        SC(sqlite3_bind_int(st_DiffInsertQuery, 6, Diff->TotalHolds));
        SC(sqlite3_bind_int(st_DiffInsertQuery, 7, Diff->TotalNotes));
        SC(sqlite3_bind_double(st_DiffInsertQuery, 8, Diff->Duration));

        if (Mode == MODE_VSRG)
        {
            VSRG::Difficulty *VDiff = static_cast<VSRG::Difficulty*>(Diff);

            SC(sqlite3_bind_int(st_DiffInsertQuery, 9, VDiff->IsVirtual));
            SC(sqlite3_bind_int(st_DiffInsertQuery, 10, VDiff->Channels));
            SC(sqlite3_bind_int(st_DiffInsertQuery, 11, VDiff->BPMType));
            SC(sqlite3_bind_int(st_DiffInsertQuery, 12, VDiff->Level));
            SC(sqlite3_bind_text(st_DiffInsertQuery, 13, VDiff->Author.c_str(), VDiff->Author.length(), SQLITE_STATIC));
            SC(sqlite3_bind_text(st_DiffInsertQuery, 14, VDiff->Data->StageFile.c_str(), VDiff->Data->StageFile.length(), SQLITE_STATIC));
        }
        else if (Mode == MODE_DOTCUR)
        {
            SC(sqlite3_bind_int(st_DiffInsertQuery, 9, 0));
            SC(sqlite3_bind_int(st_DiffInsertQuery, 10, 0));
            SC(sqlite3_bind_int(st_DiffInsertQuery, 11, 0));
            SC(sqlite3_bind_int(st_DiffInsertQuery, 12, 0));
            SC(sqlite3_bind_text(st_DiffInsertQuery, 13, Diff->Author.c_str(), Diff->Author.length(), SQLITE_STATIC));
        }

        SCS(sqlite3_step(st_DiffInsertQuery));
        SC(sqlite3_reset(st_DiffInsertQuery));
    }
    else
    {
        SC(sqlite3_bind_text(st_DiffUpdateQuery, 1, Diff->Name.c_str(), Diff->Name.length(), SQLITE_STATIC));
        SC(sqlite3_bind_int(st_DiffUpdateQuery, 2, Diff->TotalObjects));
        SC(sqlite3_bind_int(st_DiffUpdateQuery, 3, Diff->TotalScoringObjects));
        SC(sqlite3_bind_int(st_DiffUpdateQuery, 4, Diff->TotalHolds));
        SC(sqlite3_bind_int(st_DiffUpdateQuery, 5, Diff->TotalNotes));
        SC(sqlite3_bind_double(st_DiffUpdateQuery, 6, Diff->Duration));

        if (Mode == MODE_VSRG)
        {
            VSRG::Difficulty *VDiff = static_cast<VSRG::Difficulty*>(Diff);
            assert(VDiff->Data != NULL);

            SC(sqlite3_bind_int(st_DiffUpdateQuery, 7, VDiff->IsVirtual));
            SC(sqlite3_bind_int(st_DiffUpdateQuery, 8, VDiff->Channels));
            SC(sqlite3_bind_int(st_DiffUpdateQuery, 9, VDiff->BPMType));
            SC(sqlite3_bind_int(st_DiffUpdateQuery, 10, VDiff->Level));
            SC(sqlite3_bind_text(st_DiffUpdateQuery, 11, VDiff->Author.c_str(), VDiff->Author.length(), SQLITE_STATIC));
            SC(sqlite3_bind_text(st_DiffUpdateQuery, 12, VDiff->Data->StageFile.c_str(), VDiff->Data->StageFile.length(), SQLITE_STATIC));
        }
        else if (Mode == MODE_DOTCUR)
        {
            SC(sqlite3_bind_int(st_DiffUpdateQuery, 7, 0));
            SC(sqlite3_bind_int(st_DiffUpdateQuery, 8, 0));
            SC(sqlite3_bind_int(st_DiffUpdateQuery, 9, 0));

            SC(sqlite3_bind_int(st_DiffUpdateQuery, 10, 0));
            SC(sqlite3_bind_text(st_DiffUpdateQuery, 11, Diff->Author.c_str(), Diff->Author.length(), SQLITE_STATIC));
            SC(sqlite3_bind_text(st_DiffUpdateQuery, 12, "", 0, SQLITE_STATIC));
        }

        SC(sqlite3_bind_int(st_DiffUpdateQuery, 12, DiffID));
        SCS(sqlite3_step(st_DiffUpdateQuery));
        SC(sqlite3_reset(st_DiffUpdateQuery));
    }

    if (DiffID == 0)
    {
        if (!DifficultyExists(FileID, Diff->Name, &DiffID))
            Utility::DebugBreak();
        if (DiffID == 0)
            Utility::DebugBreak();
    }

    Diff->ID = DiffID;
}

std::filesystem::path SongDatabase::GetDifficultyFilename(int ID)
{
    int ret;
    SC(sqlite3_bind_int(st_GetDiffFilename, 1, ID));
    SCS(sqlite3_step(st_GetDiffFilename));

#ifdef _WIN32
    std::filesystem::path out = Utility::Widen((char*)sqlite3_column_text(st_GetDiffFilename, 0));
#else
	std::filesystem::path out = (char*)sqlite3_column_text(st_GetDiffFilename, 0)
#endif
    SC(sqlite3_reset(st_GetDiffFilename));
    return out;
}

bool SongDatabase::CacheNeedsRenewal(std::filesystem::path Dir)
{
	// must match what we put at InsertFilename time, so turn into absolute path on both places!
	std::string u8p = Utility::Narrow(std::filesystem::absolute(Dir).wstring());
    int CurLMT = Utility::GetLMT(Dir);
    bool NeedsRenewal;
    int res, ret;

    SC(sqlite3_bind_text(st_LMTQuery, 1, u8p.c_str(), u8p.length(), SQLITE_STATIC));
    res = sqlite3_step(st_LMTQuery);

    if (res == SQLITE_ROW) // entry exists
    {
        int OldLMT = sqlite3_column_int(st_LMTQuery, 0);
        bool IsLMTCurrent = (CurLMT == OldLMT); // file was not modified since last time
        NeedsRenewal = !IsLMTCurrent;
    }
    else
    {
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

std::string SongDatabase::GetArtistForDifficulty(int ID)
{
    int rs;
    std::string out;

    sqlite3_bind_int(st_GetDiffAuthor, 1, ID);
    rs = sqlite3_step(st_GetDiffAuthor);

    if (rs == SQLITE_ROW)
        out = (char*)sqlite3_column_text(st_GetDiffAuthor, 0);

    sqlite3_reset(st_GetDiffAuthor);
    return out;
}

void SongDatabase::GetSongInformation7K(int ID, VSRG::Song* Out)
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
    Out->Subtitle = (char*)sqlite3_column_text(st_GetSongInfo, 3);
    Out->BackgroundFilename = (char*)sqlite3_column_text(st_GetSongInfo, 4);
    Out->ID = ID;
    int mode = sqlite3_column_int(st_GetSongInfo, 5);
    Out->PreviewTime = sqlite3_column_double(st_GetSongInfo, 6);

    SC(sqlite3_reset(st_GetSongInfo));

    if (mode != MODE_VSRG)
        return; // Sowwy.

    // Now, difficulty information.
    SC(sqlite3_bind_int(st_GetDiffInfo, 1, ID));
    while (sqlite3_step(st_GetDiffInfo) != SQLITE_DONE)
    {
        std::shared_ptr<VSRG::Difficulty> Diff(new VSRG::Difficulty);

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
        Diff->BPMType = (VSRG::Difficulty::ETimingType)colInt;

        // We don't include author information to force querying it from the database.
        // Diff->Author
        Diff->Level = sqlite3_column_int(st_GetDiffInfo, 11);

        // File ID associated data
        int FileID = sqlite3_column_int(st_GetDiffInfo, 9);

        SC(sqlite3_bind_int(st_GetFileInfo, 1, FileID));
        sqlite3_step(st_GetFileInfo);

		// This copy is dangerous, so we should reset the info _before_ we try to copy.
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
			SC(sqlite3_reset(st_GetDiffInfo));
			throw e;
		}

		SC(sqlite3_reset(st_GetFileInfo));
        Out->Difficulties.push_back(Diff);
    }

    SC(sqlite3_reset(st_GetDiffInfo));
}

int SongDatabase::GetSongIDForFile(std::filesystem::path File, VSRG::Song* In)
{
    int ret;
    int Out = -1;
	std::string u8path = Utility::Narrow(std::filesystem::absolute(File).wstring());
    SC(sqlite3_bind_text(st_GetSIDFromFilename, 1, u8path.c_str(), u8path.length(), SQLITE_STATIC));

    int r = sqlite3_step(st_GetSIDFromFilename);
    if (r == SQLITE_ROW)
    {
        // We found a song with ID and everything..
        Out = sqlite3_column_int(st_GetSIDFromFilename, 0);
    }
    else
    {
        assert(In); // Okay, this is a query isn't it? Why doesn't the song exist?
        // Okay then, insert the song.
        // So now the latest entry is what we're going to insert difficulties and files into.
        SC(sqlite3_bind_text(st_SngInsertQuery, 1, In->SongName.c_str(), In->SongName.length(), SQLITE_STATIC));
        SC(sqlite3_bind_text(st_SngInsertQuery, 2, In->SongAuthor.c_str(), In->SongAuthor.length(), SQLITE_STATIC));
        SC(sqlite3_bind_text(st_SngInsertQuery, 3, In->Subtitle.c_str(), In->Subtitle.length(), SQLITE_STATIC));
        SC(sqlite3_bind_text(st_SngInsertQuery, 4, In->SongFilename.c_str(), In->SongFilename.length(), SQLITE_STATIC));
        SC(sqlite3_bind_text(st_SngInsertQuery, 5, In->BackgroundFilename.c_str(), In->BackgroundFilename.length(), SQLITE_STATIC));
        SC(sqlite3_bind_int(st_SngInsertQuery, 6, In->Mode));
        SC(sqlite3_bind_text(st_SngInsertQuery, 7, In->SongPreviewSource.c_str(), In->SongPreviewSource.length(), SQLITE_STATIC));
        SC(sqlite3_bind_double(st_SngInsertQuery, 8, In->PreviewTime));

        SCS(sqlite3_step(st_SngInsertQuery));
        SC(sqlite3_reset(st_SngInsertQuery));

        sqlite3_step(st_GetLastSongID);
        Out = sqlite3_column_int(st_GetLastSongID, 0);
        sqlite3_reset(st_GetLastSongID);
    }

    sqlite3_reset(st_GetSIDFromFilename);
    if (In) In->ID = Out;
    return Out;
}

std::string SongDatabase::GetStageFile(int DiffID)
{
    int ret;
    SC(sqlite3_bind_int(st_GetStageFile, 1, DiffID));
    SCS(sqlite3_step(st_GetStageFile));

    const char* sOut = (const char*)sqlite3_column_text(st_GetStageFile, 0);
    std::string Out = sOut ? sOut : "";

    SC(sqlite3_reset(st_GetStageFile));
    return Out;
}

void SongDatabase::GetPreviewInfo(int SongID, std::string &Filename, float &PreviewStart)
{
    int ret;
    SC(sqlite3_bind_int(st_GetPreviewInfo, 1, SongID));
    SCS(sqlite3_step(st_GetPreviewInfo));

    const char* sOut = (const char*)sqlite3_column_text(st_GetPreviewInfo, 0);
    float fOut = sqlite3_column_double(st_GetPreviewInfo, 1);

    std::string Out = sOut ? sOut : "";

    SC(sqlite3_reset(st_GetPreviewInfo));
    Filename = Out;
    PreviewStart = fOut;
}