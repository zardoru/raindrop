
#ifndef SONGWHEEL_H_
#define SONGWHEEL_H_

#include <boost/function.hpp>
#include <boost/thread.hpp>

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

	typedef boost::function<void (shared_ptr<Game::Song>, uint8)> SongNotification;
	typedef boost::function<void(uint32, GString, shared_ptr<Game::Song>)> ItemNotification;
	typedef boost::function<void(Sprite*, shared_ptr<Game::Song>, bool)> ItemTransformFunction;
	typedef boost::function <float (float)> ListTransformFunction;
	typedef boost::function<void()> DirectoryChangeNotifyFunction;

class SongWheel
{
private:
	friend class ScreenSelectMusic;
	SongWheel();

	int32 CursorPos, OldCursorPos;
	uint32 SelectedItem;
	int StartIndex, EndIndex;

	TruetypeFont* mTFont;
	BitmapFont* mFont;
	boost::mutex* mLoadMutex;
	boost::thread* mLoadThread;

	SongDatabase* DB;

	shared_ptr<SongList> ListRoot;
	SongList* CurrentList;

	float CurrentVerticalDisplacement;
	float PendingVerticalDisplacement;
	float shownListY;

	Sprite* Item;
	
	Vec2 ItemTextOffset;

	float ItemHeight;
	float Time;
	float DisplacementSpeed;

	void DisplayItem(int32 ListItem, Vec2 Position);
	bool InWheelBounds(Vec2 Pos);

	bool IsInitialized; 
	bool dotcurModeActive;
	bool VSRGModeActive;
	bool IsHovering;

	uint8 DifficultyIndex;

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

	float ScrollSpeed;

	// Singleton
	static SongWheel& GetInstance();

	void GoUp();
	void Initialize(float Start, float End, SongDatabase* Database, bool IsGraphical = true);

	void Join();

	bool HandleInput(int32 key, KeyEventType code, bool isMouseInput);
	bool HandleScrollInput(const double dx, const double dy);
	shared_ptr<Game::Song> GetSelectedSong();
	void ReloadSongs();

	// return: the new difficulty index
	int NextDifficulty();
	int PrevDifficulty();
	int GetDifficulty() const;
	void SetDifficulty(uint32 i);

	// Returns the index of the last item the user hovered with the mouse over.
	uint32 GetCursorIndex();
	void SetCursorIndex(int32 Index);

	void  SetFont(Directory FontDirectory);
	void  SetItemHeight(float Height);

	void  SetSelectedItem(uint32 Item);
	int32 GetSelectedItem();
	int32 GetNumItems();

	float GetListY() const;
	void SetListY(float newLY);

	float GetDeltaY() const;
	void SetDeltaY(float newDY);

	float GetTransformedY() const;

	int32 IndexAtPoint(float Y);
	uint32 NormalizedIndexAtPoint(float Y);

	float GetItemHeight();

	bool IsLoading();

	void Update(float Delta);
	void Render();
};

}

#endif