#ifdef WIN32
#include <Windows.h>
#endif

#include "Global.h"
#include "Screen.h"
#include "Directory.h"
#include "Application.h"

//#if (!defined WIN32) || (!defined NDEBUG)
int32 main (int argc, char *argv[])
/*#else
int CALLBACK WinMain (
    __in HINSTANCE hInstance,
    __in_opt HINSTANCE hPrevInstance,
    __in_opt LPSTR lpCmdLine,
    __in int nShowCmd
    )
#endif
	*/
{
	Application App(argc, argv);
	App.Init();
	App.Run();
	App.Close();
	return 0;
}
