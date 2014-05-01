#ifdef WIN32
#include <Windows.h>
#endif

#include "Global.h"
#include <exception>
#include "Screen.h"
#include "Application.h"

Application App;

//#if (!defined WIN32) || (!defined NDEBUG)
int32 main ()
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
	App.Init();
	App.Run();
	App.Close();
	return 0;
}
