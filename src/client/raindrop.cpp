#include <exception>
#include <atomic>
#include <memory>
#include <string>
#include <filesystem>

#include "structure/Screen.h"
#include "Application.h"


// ErrorReporting.cpp
void register_signals();

#ifdef _WIN32
#include <windows.h>
void PrintTraceFromContext(CONTEXT &ctx);
#endif


int main(int argc, char *argv[])
{
	register_signals();
	
#if _WIN32 && NDEBUG
	int cd = 0;
	_EXCEPTION_POINTERS *ex_info;
	__try {
#endif
	    Application app(argc, argv);
	    app.init();
	    app.run();
	    app.close();
#if _WIN32 && NDEBUG
	} 
	__except ( ex_info = GetExceptionInformation(),
		GetExceptionCode() == STATUS_ACCESS_VIOLATION ||
		GetExceptionCode() == STATUS_ARRAY_BOUNDS_EXCEEDED) {
		PrintTraceFromContext(*ex_info->ContextRecord);

		MessageBox(nullptr, 
			L"raindrop has crashed. Please see the log for details and maybe send it to github with a report of what you were doing to try and fix it.",
			L"Crash!", MB_OK | MB_ICONERROR);
	}
#endif

    return 0;
}