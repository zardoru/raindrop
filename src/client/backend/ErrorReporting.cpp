#ifdef WIN32
#include <windows.h>
#endif

#include <string>

#include <vector>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include "Logging.h"

void PrintStackTrace();

#ifdef _WIN32
void InitDbgHelp();
#endif

namespace {

volatile std::sig_atomic_t handling_signal = 0;

const char* signal_name(const int sig)
{
	switch (sig) {
	case SIGABRT: return "SIGABRT";
	case SIGSEGV: return "SIGSEGV";
#ifdef SIGBUS
	case SIGBUS: return "SIGBUS";
#endif
#ifdef SIGILL
	case SIGILL: return "SIGILL";
#endif
#ifdef SIGFPE
	case SIGFPE: return "SIGFPE";
#endif
#ifdef SIGTRAP
	case SIGTRAP: return "SIGTRAP";
#endif
	default: return "unknown";
	}
}

}

void signalrec(int sig) {
	if (handling_signal)
		std::_Exit(128 + sig);

	handling_signal = 1;
	Log::LogPrintf("Caught signal %d (%s).\n", sig, signal_name(sig));
	PrintStackTrace();

	std::signal(sig, SIG_DFL);
	std::raise(sig);
}

void register_signals() {
	signal(SIGABRT, signalrec);
	signal(SIGSEGV, signalrec);
#ifdef SIGBUS
	signal(SIGBUS, signalrec);
#endif
#ifdef SIGILL
	signal(SIGILL, signalrec);
#endif
#ifdef SIGFPE
	signal(SIGFPE, signalrec);
#endif
#ifdef SIGTRAP
	signal(SIGTRAP, signalrec);
#endif

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
#include <cstddef>
#include <cxxabi.h>
#include <dlfcn.h>
#include <execinfo.h>

namespace {

std::string demangle_symbol(const char* name)
{
	int status = 0;
	char* demangled = abi::__cxa_demangle(name, nullptr, nullptr, &status);
	if (status == 0 && demangled) {
		std::string result = demangled;
		std::free(demangled);
		return result;
	}

	std::free(demangled);
	return name ? name : "";
}

}

void PrintStackTrace() {
	constexpr int max_frames = 128;
	void* frames[max_frames];
	const int frame_count = backtrace(frames, max_frames);

	Log::LogPrintf("Stack trace (%d frames):\n", frame_count);

	char** fallback_symbols = backtrace_symbols(frames, frame_count);
	for (int i = 0; i < frame_count; ++i) {
		Dl_info info;
		std::memset(&info, 0, sizeof(info));

		if (dladdr(frames[i], &info) && info.dli_sname) {
			const auto symbol = demangle_symbol(info.dli_sname);
			const auto offset = static_cast<std::ptrdiff_t>(
				static_cast<char*>(frames[i]) - static_cast<char*>(info.dli_saddr));
			Log::LogPrintf("\t#%02d %p %s + %td (%s)\n",
				i,
				frames[i],
				symbol.c_str(),
				offset,
				info.dli_fname ? info.dli_fname : "unknown");
		}
		else if (fallback_symbols) {
			Log::LogPrintf("\t#%02d %s\n", i, fallback_symbols[i]);
		}
		else {
			Log::LogPrintf("\t#%02d %p\n", i, frames[i]);
		}
	}

	std::free(fallback_symbols);
}
#endif
