#pragma once

class GameWindow;
struct GameParameters;

namespace dotcur
{
    class Song;
}

namespace VSRG
{
    struct Difficulty;
    class Song;
}

class SongDatabase;
class Image;
class ScoreKeeper7K;

struct lua_State;

namespace Game
{
    class Song;

    class GameState
    {
        std::string CurrentSkin;
        SongDatabase* Database;

        Image* StageImage;
        Image* SongBG;
        std::shared_ptr<Game::Song> SelectedSong;
        std::shared_ptr<ScoreKeeper7K> SKeeper7K;
        std::shared_ptr<GameParameters> Params;
        std::map<std::string, std::vector<std::string>> Fallback; // 2nd is 1st's fallback

		int CurrentGaugeType;
        bool FileExistsOnSkin(const char* Filename, const char* Skin);
    public:

        GameState();
        std::string GetSkinScriptFile(const char* Filename, const std::string& Skin);
        std::shared_ptr<Game::Song> GetSelectedSongShared();
        std::string GetFirstFallbackSkin();
        static GameState &GetInstance();
        void Initialize();

        void InitializeLua(lua_State *L);

        std::string GetDirectoryPrefix();
        std::string GetSkinPrefix();
        std::string GetSkinPrefix(const std::string &skin);
        std::string GetScriptsDirectory();
        void SetSkin(std::string NextSkin);
        Image* GetSkinImage(const std::string& Image);
        bool SkinSupportsChannelCount(int Count);
        std::string GetSkin();

        void SetSelectedSong(std::shared_ptr<Game::Song> Song);
        Game::Song *GetSelectedSong();
        void SetDifficultyIndex(uint32_t DifficultyIndex);
        uint32_t GetDifficultyIndex() const;

		void SetCurrentGaugeType(int GaugeType);
		int GetCurrentGaugeType() const;


        // Note: Returning a shared_ptr causes lua to fail an assertion, since shared_ptr is not registered.
        ScoreKeeper7K* GetScorekeeper7K();
        void SetScorekeeper7K(std::shared_ptr<ScoreKeeper7K> Other);

        std::string GetSkinFile(const std::string &Name, const std::string &Skin);
        std::string GetSkinFile(const std::string &Name);
        std::string GetFallbackSkinFile(const std::string &Name);

        SongDatabase* GetSongDatabase();
        static GameWindow* GetWindow();

        GameParameters* GetParameters();
    };
}

using Game::GameState;

// This loads the meta only from the database.
void LoadSong7KFromDir(Directory songPath, std::vector<VSRG::Song*> &VecOut);

// This loads the whole of the song.
std::shared_ptr<VSRG::Song> LoadSong7KFromFilename(const std::filesystem::path&, VSRG::Song *Sng);

// Loads the whole of the song.
void LoadSongDCFromDir(Directory songPath, std::vector<dotcur::Song*> &VecOut);
