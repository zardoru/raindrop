#ifdef WIN32
#include <windows.h>
#endif

#include <string>

#include <vector>
#include <csignal>
#include "Logging.h"

void PrintStackTrace();

#ifdef _WIN32
void InitDbgHelp();
#endif

void signalrec(int sig) {
	PrintStackTrace();
}

void RegisterSignals() {
	signal(SIGABRT, signalrec);
	signal(SIGSEGV, signalrec);

#ifdef _WIN32
#ifndef NDEBUG
	Log::LogPrintf("Initializing DbgHelp.\n");
#endif
	InitDbgHelp();
#endif
}


#ifdef _WIN32
#include <DbgHelp.h>
#include <TlHelp32.h>

void InitDbgHelp()
{
	static bool inited = false;
	if (!inited) {
		SymInitialize(GetCurrentProcess(), NULL, TRUE);
		SymSetOptions(SYMOPT_LOAD_LINES);
		inited = true;
	}
}

std::vector<DWORD> GetAllThreadIDs()
{
	auto ret = std::vector<DWORD>();
	auto snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	auto pid = GetCurrentProcessId();

	THREADENTRY32 tentry;
	tentry.dwSize = sizeof(THREADENTRY32);
	if (Thread32First(snapshot, &tentry)) {
		do {
			if (pid == tentry.th32OwnerProcessID)
				ret.push_back(tentry.th32ThreadID);
		} while (Thread32Next(snapshot, &tentry));
	}

	CloseHandle(snapshot);
	return ret;
}

void PrintTraceFromContext(CONTEXT &ctx)
{
	auto handle = GetCurrentProcess();

	auto thIDs = GetAllThreadIDs();

	for (auto tid : thIDs) {
		auto thread_handle = OpenThread(READ_CONTROL, false, tid);

		STACKFRAME64 stackframe;
		memset(&stackframe, 0, sizeof(STACKFRAME64));
		stackframe.AddrPC.Mode = AddrModeFlat;
		stackframe.AddrFrame.Mode = AddrModeFlat;
		stackframe.AddrStack.Mode = AddrModeFlat;

#if _WIN64
        stackframe.AddrPC.Offset = ctx.Rip;
        stackframe.AddrFrame.Offset = ctx.Rsp;
        stackframe.AddrStack.Offset = ctx.Rsp;
#else
        stackframe.AddrPC.Offset = ctx.Eip;
		stackframe.AddrFrame.Offset = ctx.Ebp;
		stackframe.AddrStack.Offset = ctx.Esp;
#endif
		Log::LogPrintf("Stack trace for thread %d:\n", tid);
		// Eh.. 32 frames?
		while (true) {

#if _WIN64
		    auto img = IMAGE_FILE_MACHINE_AMD64;
#else
            auto img = IMAGE_FILE_MACHINE_I386;
#endif
			if (!StackWalk64(img,
				handle,
				thread_handle,
				&stackframe,
				&ctx,
				NULL,
				SymFunctionTableAccess64,
				SymGetModuleBase64,
				NULL)) {
				auto err = GetLastError();
				if (err)
					Log::LogPrintf("StalkWalk64 error: %d\n", err);
				break;
			}

			if (stackframe.AddrPC.Offset != 0) {
				auto len = sizeof(PIMAGEHLP_SYMBOL64) + 255 * sizeof(TCHAR);
				PIMAGEHLP_SYMBOL64 sym = (PIMAGEHLP_SYMBOL64)malloc(len);
				memset(sym, 0, len);
				sym->MaxNameLength = 255;
				sym->SizeOfStruct = sizeof(SYMBOL_INFOW);

				DWORD64 displ = 0;
				char undecorated[256];
				if (SymGetSymFromAddr64(handle, stackframe.AddrPC.Offset, &displ, sym)) {
					UnDecorateSymbolName((const char*)sym->Name, undecorated, 255, UNDNAME_COMPLETE);

					displ = 0;
					IMAGEHLP_LINE64 line;
					line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

					DWORD displine = 0;
					if (SymGetLineFromAddr64(GetCurrentProcess(), stackframe.AddrPC.Offset, &displine, &line))
						Log::LogPrintf("\tPC: %08I64x Ret: %08I64x Frame: %08I64x @ %s (%s line %d)\n",
							stackframe.AddrPC.Offset,
							stackframe.AddrReturn.Offset,
							stackframe.AddrFrame.Offset,
							undecorated, line.FileName, line.LineNumber);
					else
						Log::LogPrintf("\tPC: %08I64x Ret: %08I64x Frame: %08I64x @ %s\n",
							stackframe.AddrPC.Offset,
							stackframe.AddrReturn.Offset,
							stackframe.AddrFrame.Offset,
							undecorated);
				}
				else {
					auto err = GetLastError();
					Log::LogPrintf("\tError calling SymFromAddr: %d\n", err);
				}
				free(sym);
			}
			else
				break;

		}
	}
}

// Sources: https://jpassing.com/2008/03/12/walking-the-stack-of-the-current-thread/
// http://www.debuginfo.com/examples/src/StackWalk.cpp
// http://stackoverflow.com/questions/5705650/stackwalk64-on-windows-get-symbol-name 
// http://code-freeze.blogspot.cl/2012/01/generating-stack-traces-from-c.html 
void PrintStackTrace() {
	/*
		Next in "A series of questionable API decisions": StackWalk!
	*/


	CONTEXT ctx;
	memset(&ctx, 0, sizeof(CONTEXT));
	ctx.ContextFlags = CONTEXT_CONTROL;
	RtlCaptureContext(&ctx);

	PrintTraceFromContext(ctx);
}
#else
void PrintStackTrace() {

}
#endif