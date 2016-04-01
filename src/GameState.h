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
		int CurrentScoreType;
		int CurrentSubsystemType;

        bool FileExistsOnSkin(const char* Filename, const char* Skin);
    public:

        GameState();
        std::string GetSkinScriptFile(const char* Filename, const std::string& Skin);
        std::shared_ptr<Game::Song> GetSelectedSongShared() const;
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
        Game::Song *GetSelectedSong() const;
        void SetDifficultyIndex(uint32_t DifficultyIndex);
        uint32_t GetDifficultyIndex() const;

		// VSRG Gauge Type
		void SetCurrentGaugeType(int GaugeType);
		int GetCurrentGaugeType() const;
	    Image* GetSongBG();
	    Image* GetSongStage();
	    // VSRG score system
		void SetCurrentScoreType(int ScoreType);
		int GetCurrentScoreType() const;

		// VSRG subsystem
		void SetCurrentSystemType(int SystemType);
		int GetCurrentSystemType() const;

        // Note: Returning a shared_ptr causes lua to fail an assertion, since shared_ptr is not registered.
        ScoreKeeper7K* GetScorekeeper7K();
        void SetScorekeeper7K(std::shared_ptr<ScoreKeeper7K> Other);

        std::filesystem::path GetSkinFile(const std::string &Name, const std::string &Skin);
        std::filesystem::path GetSkinFile(const std::string &Name);
        std::filesystem::path GetFallbackSkinFile(const std::string &Name);

        SongDatabase* GetSongDatabase();
        static GameWindow* GetWindow();

		void SubmitScore(std::shared_ptr<ScoreKeeper7K> score);

        GameParameters* GetParameters();
    };
}

using Game::GameState;