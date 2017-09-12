#pragma once
#include "SongList.h"

namespace Game {
	namespace VSRG
	{
		class Song;
	}
}

class BitmapFont;
class Sprite;
class SongDatabase;
class TruetypeFont;
class LuaManager;
class GraphicalString;

/*
    The flow of the wheel is as follows.

    When the mouse hovers over an item, the user is notified. When it clicks, the user is notified too.
    The user decides on these two events whether the index is selected or not.
    Hover is called only once every change. Click is only called when the assigned buttons to KT_Select are pressed.

    When an item is selected once and it's a directory, we move over to it.
    If it's not, we set it as a tentative song pick. If it's selected again, we confirm a song was selected.
*/

namespace Game
{
    typedef std::function<void(std::shared_ptr<VSRG::Song>, uint8_t)> SongNotification;
    typedef std::function<void(int32_t, uint32_t, std::string, std::shared_ptr<Song>)> ItemNotification;
    typedef std::function<void(int32_t, std::shared_ptr<VSRG::Song>, bool, int32_t)> ItemTransformFunction;
    typedef std::function<void(int32_t, std::shared_ptr<VSRG::Song>, bool, int32_t, std::string)> StringTransformFunction;
    typedef std::function <float(float)> ListTransformFunction;
    typedef std::function<void()> DirectoryChangeNotifyFunction;
	typedef std::function<bool(const ListEntry * const)> FuncFilterCriteria;

    class SongWheel
    {
    private:
        friend class ScreenSelectMusic;
        SongWheel();

        int32_t CursorPos, OldCursorPos;
        int32_t SelectedBoundItem, SelectedUnboundItem;

        std::mutex* mLoadMutex;
        std::thread* mLoadThread;
        std::atomic<bool> mLoading;

        SongDatabase* DB;

        std::shared_ptr<SongList> ListRoot;
        SongList* CurrentList;
		SongList FilteredCurrentList;

        std::map<int, Sprite*> Sprites;
        std::map<int, GraphicalString*> Strings;

        float Time;

		// itemFraction = item / total of displayed items
        void DisplayItem(int32_t ListItem, int32_t ItemPosition, float itemFraction);
        bool InWheelBounds(Vec2 Pos);

        bool IsInitialized;
        bool dotcurModeActive;
        bool VSRGModeActive;
        bool IsHovering;

        bool LoadedSongsOnce;
        size_t DifficultyIndex;

		std::vector<FuncFilterCriteria> ActiveFilters;

		AABBd ItemBoxAt(float t);
    public:
		
        DirectoryChangeNotifyFunction OnDirectoryChange;

        ItemNotification OnItemClick;
        ItemNotification OnItemHover;
        ItemNotification OnItemHoverLeave;

        SongNotification OnSongConfirm;
        SongNotification OnSongTentativeSelect;

        ListTransformFunction TransformHorizontal;
		ListTransformFunction TransformVertical;
		ListTransformFunction TransformWidth;
		ListTransformFunction TransformHeight;

		int DisplayItemCount;
		int DisplayStartIndex;

        ItemTransformFunction TransformItem;
        StringTransformFunction TransformString;

        // Singleton
        static SongWheel& GetInstance();

        void CleanItems();

		void ReapplyFilters();

        void GoUp();
        void Initialize(SongDatabase* Database);

        void Join();

        bool HandleInput(int32_t key, KeyEventType code, bool isMouseInput);
        bool HandleScrollInput(const double dx, const double dy);
        std::shared_ptr<VSRG::Song> GetSelectedSong();
        void ReloadSongs(SongDatabase* Database);
        void LoadSongsOnce(SongDatabase* Database);

        int AddSprite(Sprite* Item);
        int AddText(GraphicalString* Str);

        // return: the new difficulty index
        int NextDifficulty();
        int PrevDifficulty();
        int GetDifficulty() const;
        void SetDifficulty(uint32_t i);

        // Returns the index of the last item the user hovered with the mouse over.
        int GetCursorIndex() const;
        void SetCursorIndex(int Index);

        void ConfirmSelection();

        // Returns the item index the mouse is currently hovering over.
        int GetListCursorIndex() const;

        // These give and set the global, infinite wheel item.
        // When wanting to use the bound index, read from SelectedBoundItem, not these.
        void  SetSelectedItem(int32_t Item);
        int32_t GetSelectedItem() const;
        int32_t GetNumItems() const;

		bool IsItemDirectory(int32_t Item) const;

        int32_t IndexAtPoint(float X, float Y);
        uint32_t NormalizedIndexAtPoint(float X, float Y);

        bool IsLoading();

		void SortBy(ESortCriteria criteria);

		void ResetFilters();
		void SelectBy(FuncFilterCriteria criteria);

        void Update(float Delta);
        void Render();
    };
}