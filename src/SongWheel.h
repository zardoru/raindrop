#pragma once

namespace dotcur
{
	class Song;
}

namespace VSRG
{
	class Song;
}

class BitmapFont;
class Sprite;
class SongList;
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

	typedef std::function<void (std::shared_ptr<Song>, uint8_t)> SongNotification;
	typedef std::function<void(int32_t, uint32_t, GString, std::shared_ptr<Song>)> ItemNotification;
	typedef std::function<void(int32_t, std::shared_ptr<Song>, bool, int32_t)> ItemTransformFunction;
	typedef std::function<void(int32_t, std::shared_ptr<Song>, bool, int32_t, GString)> StringTransformFunction;
	typedef std::function <float (float)> ListTransformFunction;
	typedef std::function<void()> DirectoryChangeNotifyFunction;

class SongWheel
{
private:
	friend class ScreenSelectMusic;
	SongWheel();

	int32_t CursorPos, OldCursorPos;
	int32_t SelectedItem, SelectedListItem;
	int StartIndex, EndIndex;

	std::mutex* mLoadMutex;
	std::thread* mLoadThread;
	std::atomic<bool> mLoading;

	SongDatabase* DB;

    std::shared_ptr<SongList> ListRoot;
	SongList* CurrentList;

	float CurrentVerticalDisplacement;
	float PendingVerticalDisplacement;
	float shownListY;

	std::map<int, Sprite*> Sprites;
	std::map<int, GraphicalString*> Strings;
	
	float ItemHeight;
	float Time;
	float DisplacementSpeed;
	float ItemWidth;
	void DisplayItem(int32_t ListItem, int32_t ItemPosition, Vec2 Position);
	bool InWheelBounds(Vec2 Pos);

	bool IsInitialized; 
	bool dotcurModeActive;
	bool VSRGModeActive;
	bool IsHovering;

	bool LoadedSongsOnce;
	uint8_t DifficultyIndex;

	// We need to find the start and the end indices of what we want to display.
	void CalculateIndices();
public:

	DirectoryChangeNotifyFunction OnDirectoryChange;

	ItemNotification OnItemClick;
	ItemNotification OnItemHover;
	ItemNotification OnItemHoverLeave;

	SongNotification OnSongConfirm;
	SongNotification OnSongTentativeSelect;

	ListTransformFunction TransformHorizontal;
	ListTransformFunction TransformListY;
	ListTransformFunction TransformPendingDisplacement;

	ItemTransformFunction TransformItem;								 
	StringTransformFunction TransformString;

	float ScrollSpeed;

	// Singleton
	static SongWheel& GetInstance();

	void CleanItems();

	void GoUp();
	void Initialize(SongDatabase* Database);

	void Join();

	bool HandleInput(int32_t key, KeyEventType code, bool isMouseInput);
	bool HandleScrollInput(const double dx, const double dy);
    std::shared_ptr<Song> GetSelectedSong();
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

	void  SetItemHeight(float Height);

	void SetItemWidth(float width);
	float GetItemWidth() const;

	// These give and set the global, infinite wheel item.
	// When wanting to use the bound index, read from SelectedItem, not these.
	void  SetSelectedItem(int32_t Item);
	int32_t GetSelectedItem() const;
	int32_t GetNumItems() const;

	bool IsItemDirectory(int32_t Item);

	float GetListY() const;
	void SetListY(float newLY);

	float GetDeltaY() const;
	void SetDeltaY(float newDY);

	float GetTransformedY() const;

	int32_t IndexAtPoint(float Y);
	uint32_t NormalizedIndexAtPoint(float Y);

	float GetItemHeight() const;

	bool IsLoading();

	void Update(float Delta);
	void Render();
};

}