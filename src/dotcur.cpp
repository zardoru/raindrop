#ifdef WIN32
#include <Windows.h>
#endif

#include "Global.h"
#include <exception>
#include "Screen.h"
#include "Application.h"

Application App;

#ifndef WIN32
int32 main ()
#else
int CALLBACK WinMain (
    __in HINSTANCE hInstance,
    __in_opt HINSTANCE hPrevInstance,
    __in_opt LPSTR lpCmdLine,
    __in int nShowCmd
    )
#endif
{

#ifdef NDEBUG
try  
#endif
	{
		App.Init();
		App.Run();
		App.Close();
	}
#ifdef NDEBUG
catch (std::exception &ex)
	{
#ifdef WIN32
		MessageBox(NULL, ex.what(), "Exception!", MB_OK);
#endif
	}
#endif
	return 0;
}