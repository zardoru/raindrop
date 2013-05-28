#include "Global.h"
#include <exception>
#include "Screen.h"
#include "Application.h"

#ifdef WIN32
#include <Windows.h>
#endif

Application App;

#ifndef WIN32
int32 main ()
#else
int CALLBACK WinMain(
  _In_  HINSTANCE hInstance,
  _In_  HINSTANCE hPrevInstance,
  _In_  LPSTR lpCmdLine,
  _In_  int nCmdShow
)
#endif
{
	try  {
		App.Init();
		App.Run();
		App.Close();
	} catch (std::exception &ex)
	{
#ifdef WIN32
		MessageBox(NULL, ex.what(), "Exception!", MB_OK);
#endif
	}
	return 0;
}