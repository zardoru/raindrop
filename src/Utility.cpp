#ifdef WIN32
#include <Windows.h>
#ifndef MINGW
#include <direct.h>
#endif
#else
#include <iconv.h>
#endif

#include <csignal>
#include <sys/stat.h>
#include <clocale>
#include <fstream>

#include "Global.h"
#include "Logging.h"
#include "sha256.h"

#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/classification.hpp>

int InfinityMask = 0x7F800000;
float *PInfinity = (float*)&InfinityMask;

namespace Utility {

	void DebugBreak()
	{
#ifndef NDEBUG
#if (defined WIN32) && !(defined MINGW)
		__asm int 3
#else
		raise(SIGTRAP);
#endif
#endif
	}

#ifndef isdigit
	bool isdigit(char s)
	{
		return s >= '0' && s <= '9';
	}
#endif

	bool IsNumeric(const char* s)
	{
		// check for first character being a minus sign
		std::stringstream k;
		double d;
		k << s;
		k >> d;
		return !k.fail();
	}

	GString GetExtension(GString Filename)
	{
		return Filename.substr(Filename.find_last_of(".")+1);
	}

	GString RelativeToPath(GString Filename)
	{
		return Filename.substr(Filename.find_last_of("/"));
	}

	GString RemoveExtension(GString Fn)
	{
		return Fn.substr(0, Fn.find_last_of("."));
	}

	bool FileExists(GString Fn)
	{
#if !(defined WIN32) || (defined MINGW)
		struct stat st;
		return (stat(Fn.c_str(), &st) != -1);
#else
		return _waccess( Utility::Widen(Fn).c_str(), 00 ) != -1;
#endif
	}

	std::wstring Widen(GString Line)
	{
		wchar_t u16s[2048]; // Ought to be enough for everyone.

#ifndef WIN32
		mbstowcs(u16s, Line.c_str(), 2048);
#else
		memset(u16s, 0, sizeof(wchar_t) * 2048);
		size_t len = MultiByteToWideChar(CP_UTF8, 0, Line.c_str(), Line.length(), u16s, 2048);
#endif
		return std::wstring(u16s);
	}

	GString Narrow(std::wstring Line)
	{
		char mbs[2048];
		
#ifndef WIN32
		wcstombs(mbs, Line.c_str(), 2048);
#else
		memset(mbs, 0, 2048);
		size_t len = WideCharToMultiByte(CP_UTF8, 0, Line.c_str(), Line.length(), mbs, 2048, NULL, NULL);
#endif
		return GString(mbs);
	}

	char* buf = (char*)malloc(2048);

	GString SJIStoU8 (GString Line)
	{
#ifdef WIN32
		wchar_t u16s[2048];
		char mbs[2048];
		size_t len = MultiByteToWideChar(932, 0, Line.c_str(), Line.length(), u16s, 2048);
		len = WideCharToMultiByte(CP_UTF8, 0, u16s, len, mbs, 2048, NULL, NULL);
		mbs[len] = 0;
		return GString(mbs);
#else
		iconv_t conv;
		char** out = &buf;
		const char* in = Line.c_str();
		size_t BytesLeftSrc = Line.length();
		size_t BytesLeftDst = 2048;

		conv = iconv_open("UTF-8", "SHIFT_JIS");
		bool success = (iconv(conv, (char **)&in, &BytesLeftSrc, out, &BytesLeftDst) > -1);

		iconv_close(conv);
		if (success)
			return GString(*out);
		else
		{
			Log::Printf("Failure converting character sets.");
			return GString();
		}
#endif
	}

	void CheckDir(GString path)
	{
		struct stat st;
		if (stat(path.c_str(), &st))
		{
#if (defined WIN32) && !(defined MINGW)
			_mkdir(path.c_str());
#else
			mkdir(path.c_str(), S_IWUSR);
#endif
		}
	}

	int GetLMT(GString Path)
	{
		struct stat st;
		if (stat(Path.c_str(), &st) != -1)
		{
			return st.st_mtime;
		}
		else return 0;
	}

	void RemoveFilenameIllegalCharacters(GString &S, bool removeSlash)
	{
		// size_t len = strlen(fn);
		boost::replace_all(S, "<", "");
		boost::replace_all(S, ">", "");
		boost::replace_all(S, ":", "");
		boost::replace_all(S, "\"", "");
		boost::replace_all(S, "|", "");
		boost::replace_all(S, "?", "");
		boost::replace_all(S, "*", "");
		if (removeSlash)
			boost::replace_all(S, "/", "");
	}

	GString getSha256ForFile(GString Filename)
	{
		SHA256 SHA;
#ifndef WIN32
		std::ifstream InStream(Filename.c_str());
#else
		std::ifstream InStream(Utility::Widen(Filename).c_str());
#endif
		unsigned char tmpbuf[256];

		if (!InStream.is_open())
			return "";

		while (!InStream.eof())
		{
			InStream.read((char*)tmpbuf, 256);
			size_t cnt = InStream.gcount();

			SHA.add(tmpbuf, cnt);
		}

		return GString(SHA.getHash());
	}

} // namespace Utility

#ifdef NDEBUG
namespace boost
{
void throw_exception(std::exception const&)
{
}
}
#endif

double latof(GString s)
{
	char point = *localeconv()->decimal_point;

	if (s.find_first_of(point) == GString::npos)
	{
		char toFind = '.';
		if (point == ',') toFind = '.';
		else if (point == '.') toFind = ',';

		size_t idx = s.find_first_of(toFind);
		if (idx != GString::npos)
			s[idx] = point;
	}

	return atof(s.c_str());
}
