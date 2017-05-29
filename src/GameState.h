#pragma once

class GameWindow;
class SongDatabase;
class Texture;
class Screen;

struct lua_State;

namespace Game
{
    class Song;

	namespace dotcur
	{
		class Song;
	}

	namespace VSRG
	{
		struct PlayscreenParameters;
		struct Difficulty;
		class Song;
		class ScoreKeeper;
	}

    class GameState
    {
        std::string CurrentSkin;
        SongDatabase* Database;

        Texture* StageImage;
        Texture* SongBG;
        std::shared_ptr<Game::Song> SelectedSong;

		struct SPlayerCurrent7K {
			std::shared_ptr<VSRG::ScoreKeeper> SKeeper7K;
			VSRG::PlayscreenParameters Params;
			std::shared_ptr<VSRG::Difficulty> Diff;

			int CurrentGaugeType;
			int CurrentScoreType;
			int CurrentSubsystemType;
			SPlayerCurrent7K::SPlayerCurrent7K (){
				CurrentGaugeType = 0;
				CurrentScoreType = 0;
				CurrentSubsystemType = 0;
			}
		};

		std::vector<SPlayerCurrent7K> PlayerInfo;

        std::map<std::string, std::vector<std::string>> Fallback; // 2nd is 1st's fallback

		
		std::shared_ptr<Screen> RootScreen;
        bool FileExistsOnSkin(const char* Filename, const char* Skin);
    public:

        GameState();
        std::filesystem::path GetSkinScriptFile(const char* Filename, const std::string& Skin);
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
        Texture* GetSkinImage(const std::string& Texture);
        bool SkinSupportsChannelCount(int Count);
        std::string GetSkin();

		void SetSelectedSong(std::shared_ptr<Game::Song> song);
        Game::Song *GetSelectedSong() const;

	    Texture* GetSongBG();
	    Texture* GetSongStage();

		void StartScreenTransition(std::string target);
		void ExitCurrentScreen();

        std::filesystem::path GetSkinFile(const std::string &Name, const std::string &Skin);
        std::filesystem::path GetSkinFile(const std::string &Name);
        std::filesystem::path GetFallbackSkinFile(const std::string &Name);

        SongDatabase* GetSongDatabase();
        static GameWindow* GetWindow();

		void SortWheelBy(int criteria);

		/* Player-number dependant functions */
		bool PlayerNumberInBounds(int pn) const;

		// VSRG Gauge Type
		void SetCurrentGaugeType(int GaugeType, int pn);
		int GetCurrentGaugeType(int pn) const;

	    // VSRG score system
		void SetCurrentScoreType(int ScoreType, int pn);
		int GetCurrentScoreType(int pn) const;

		// VSRG subsystem
		void SetCurrentSystemType(int SystemType, int pn);
		int GetCurrentSystemType(int pn) const;

        // Note: Returning a shared_ptr causes lua to fail an assertion, since shared_ptr is not registered.
        VSRG::ScoreKeeper* GetScorekeeper7K(int pn);
        void SetScorekeeper7K(std::shared_ptr<VSRG::ScoreKeeper> Other, int pn);

        VSRG::PlayscreenParameters* GetParameters(int pn);
		VSRG::Difficulty* GetDifficulty(int pn);
		std::shared_ptr<VSRG::Difficulty> GetDifficultyShared(int pn);
		void SetDifficulty(std::shared_ptr<VSRG::Difficulty> df, int pn);
		int GetPlayerCount() const;
		void SubmitScore(int pn);

		bool IsSongUnlocked(Game::Song *song);
		void UnlockSong(Game::Song *song);

		void SetRootScreen(std::shared_ptr<Screen> root);
		std::shared_ptr<Screen> GetCurrentScreen();
		std::shared_ptr<Screen> GetNextScreen();
    };
}

using Game::GameState;