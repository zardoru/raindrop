
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
class GraphObject2D;
class SongList;
class SongDatabase;
class TruetypeFont;

namespace Game 
{

	typedef boost::function<void (Game::Song*, uint8)> SongNotification;

	typedef boost::function <float (float)> ListTransformFunction;

class SongWheel
{
private:
	SongWheel();

	int32 CursorPos, OldCursorPos;
	int StartIndex, EndIndex;

	TruetypeFont* mTFont;
	BitmapFont* mFont;
	boost::mutex* mLoadMutex;
	boost::thread* mLoadThread;

	SongDatabase* DB;

	SongList* ListRoot;
	SongList* CurrentList;

	float CurrentVerticalDisplacement;
	float PendingVerticalDisplacement;

	GraphObject2D* SelCursor, *Item, *ItemDirectory;

	SongNotification OnSongChange;
	SongNotification OnSongSelect;
	ListTransformFunction Transform;

	Vec2 ItemTextOffset;

	enum
	{
		M_LESSTHAN, 
		M_GREATERTHAN,
		M_RANGE
	} IntervalType;

	float RangeStart, RangeEnd;

	float ItemHeight;
	float Time;
	float DisplacementSpeed;

	void DisplayItem(GString Text, Vec2 Position);

	bool IsInitialized; 
	bool dotcurModeActive;
	bool VSRGModeActive;

	uint8 DifficultyIndex;

	// We need to find the start and the end indices of what we want to display.
	int GetCursorIndex();
	void CalculateIndices();
public:

	// Singleton
	static SongWheel& GetInstance();

	void GoUp();
	void Initialize(float Start, float End, 
		ListTransformFunction FuncTransform, SongNotification FuncNotify, SongNotification FuncNotifySelect, 
		SongDatabase* Database);

	bool HandleInput(int32 key, KeyEventType code, bool isMouseInput);
	bool HandleScrollInput(const double dx, const double dy);
	Game::Song* GetSelectedSong();
	void ReloadSongs();

	void SetFont(Directory FontDirectory);
	void SetItemHeight(float Height);

	void Update(float Delta);
	void Render();
};

}

#endif