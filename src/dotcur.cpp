#include "Global.h"
#include <exception>
#include "Screen.h"
#include "Application.h"

#ifdef WIN32
#include <Windows.h>
#endif

Application App;

int32 main ()
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