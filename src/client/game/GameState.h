#pragma once

class SongDatabase;
class Texture;
class Screen;

struct lua_State;

namespace rd
{
    class Song;
    class Difficulty;
    class ScoreKeeper;
}

class Profile;
class PlayerContext;

namespace StormIR {
    class StormIR;
}

class GameState
{
    std::string CurrentSkin;
    SongDatabase* Database;

    Texture* StageImage;
    Texture* SongBG;
    std::shared_ptr<rd::Song> SelectedSong;
    std::unique_ptr<StormIR::StormIR> ir;

    struct SPlayerCurrent7K {
        std::shared_ptr<rd::ScoreKeeper> scorekeeper;
        PlayscreenParameters play_parameters;
        std::shared_ptr<rd::Difficulty> active_difficulty;
        PlayerContext *ctx;
        Profile* profile;
    };

    std::vector<SPlayerCurrent7K> PlayerInfo;

    std::map<std::string, std::vector<std::string>> Fallback; // 2nd is 1st's fallback


    std::shared_ptr<Screen> RootScreen;
    static bool FileExistsOnSkin(const char* Filename, const char* Skin);
public:

    GameState();
    std::filesystem::path GetSkinScriptFile(const char* Filename, const std::string& Skin);
    std::shared_ptr<rd::Song> GetSelectedSongShared() const;
    std::string GetFirstFallbackSkin();
    static GameState &GetInstance();
    void Initialize();

    /* Defines Difficulty/Song/Playscreen/Gamestate
     and defines Global as the Gamestate singleton */
    void InitializeLua(lua_State *L);

    std::string GetDirectoryPrefix();
    std::string GetSkinPrefix();
    static std::string GetSkinPrefix(const std::string &skin);
    static std::string GetScriptsDirectory();
    void SetSkin(std::string NextSkin);
    Texture* GetSkinImage(const std::string& Texture);
    static bool SkinSupportsChannelCount(int Count);
    std::string GetSkin();

    void SetSelectedSong(std::shared_ptr<rd::Song> song);
    rd::Song *GetSelectedSong() const;

    Texture* GetSongBG();
    Texture* GetSongStage();

    void StartScreenTransition(std::string target);
    void ExitCurrentScreen();

    std::filesystem::path GetSkinFile(const std::string &Name, const std::string &Skin);
    std::filesystem::path GetSkinFile(const std::string &Name);
    std::filesystem::path GetFallbackSkinFile(const std::string &Name);

    SongDatabase* GetSongDatabase();

    void SortWheelBy(int criteria);

    /* Player-number dependant functions */
    bool PlayerNumberInBounds(int pn) const;

    void SetPlayerContext(PlayerContext* pc, int pn);

    // VSRG Gauge Type
    int GetCurrentGaugeType(int pn) const;

    // VSRG score system
    int GetCurrentScoreType(int pn) const;

    // VSRG subsystem
    int GetCurrentSystemType(int pn) const;

    // Note: Returning a shared_ptr causes lua to fail an assertion, since shared_ptr is not registered.
    rd::ScoreKeeper* GetScorekeeper7K(int pn);
    void SetScorekeeper7K(std::shared_ptr<rd::ScoreKeeper> Other, int pn);

    PlayscreenParameters* GetParameters(int pn);
    rd::Difficulty* GetDifficulty(int pn);
    std::shared_ptr<rd::Difficulty> GetDifficultyShared(int pn);
    void SetDifficulty(std::shared_ptr<rd::Difficulty> df, int pn);
    int GetPlayerCount() const;
    void SubmitScore(int pn);

    bool IsSongUnlocked(rd::Song *song);
    void UnlockSong(rd::Song *song);

    void SetSystemFolder(const std::string folder);

    void SetRootScreen(std::shared_ptr<Screen> root);
    static std::shared_ptr<Screen> GetCurrentScreen();
    std::shared_ptr<Screen> GetNextScreen();

    void AddActiveProfile(std::string profile_name);
};
