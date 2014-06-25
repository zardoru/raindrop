#include <stdarg.h>
#include <fstream>

#include "GameGlobal.h"
#include "GameState.h"
#include "GameWindow.h"

using namespace Game;

GameState::GameState()
{
}

GameState& GameState::GetInstance()
{
	static GameState* StateInstance = new GameState;
	return *StateInstance;
}

void GameState::Initialize()
{
}

#ifdef DEBUG
static void DebugPrintf(String Format, ...)
{

}
#endif

void GameState::Printf(String Format, ...)
{
	char Buffer[2048];
	va_list vl;
	va_start(vl,Format);
	vsnprintf(Buffer, 2048, Format.c_str(), vl);
	va_end(vl);
	wprintf(L"%ls", Utility::Widen(Buffer).c_str());
}

void GameState::Logf(String Format, ...)
{
	static std::fstream logf ("log.txt", std::ios::out);
	char Buffer[2048];
	va_list vl;
	va_start(vl,Format);
	vsnprintf(Buffer, 2048, Format.c_str(), vl);
	va_end(vl);
	logf << Buffer;
	logf.flush();
}

GameWindow* GameState::GetWindow()
{
	return &WindowFrame;
}