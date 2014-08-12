#ifndef GAMESTATE_H_
#define GAMESTATE_H_

class GameWindow;

namespace Game
{
class GameState
{

public:

	GameState();
	static GameState &GetInstance();
	void Initialize();

	static GameWindow* GetWindow();

#ifdef DEBUG
	static void DebugPrintf(String Format, ...);
#else
#define DebugPrintf(...)
#endif
	static void Printf(String Format, ...);
	static void Logf(String Format, ...);
};
}

using Game::GameState;

#else
#error "GameState.h included twice."
#endif
