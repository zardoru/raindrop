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

#include <sstream>
#include <regex>

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

    const short MAX_STRING_SIZE = 2048;

	GString SJIStoU8 (GString Line)
	{
#ifdef WIN32
		wchar_t u16s[MAX_STRING_SIZE];
		char mbs[MAX_STRING_SIZE];
		size_t len = MultiByteToWideChar(932, 0, Line.c_str(), Line.length(), u16s, MAX_STRING_SIZE);
		len = WideCharToMultiByte(CP_UTF8, 0, u16s, len, mbs, MAX_STRING_SIZE, NULL, NULL);
		mbs[len] = 0;
		return GString(mbs);
#elif defined(DARWIN)
        // Note: for OS X/Darwin/More than likely most BSD variants, iconv behaves a bit differently.
        iconv_t conv;
        char buf[MAX_STRING_SIZE];
        char* out = buf;
        size_t srcLength = Line.length();
        size_t dstLength = MAX_STRING_SIZE;
        const char* in = Line.c_str();
        
        conv = iconv_open("UTF-8", "SHIFT_JIS");
        iconv(conv, (char**)&in, &srcLength, (char**)&out, &dstLength);
        iconv_close(conv);
        // We have to use buf instead of out here.  For whatever reason, iconv on Darwin doesn't get us what we would expect if we just use out.
        return GString(buf);
#else
        char buf[MAX_STRING_SIZE];
		iconv_t conv;
		char** out = &buf;
		const char* in = Line.c_str();
		size_t BytesLeftSrc = Line.length();
		size_t BytesLeftDst = MAX_STRING_SIZE;

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

	GString IntToStr(int num)
	{
		std::stringstream k;
		k << num;
		return k.str();
	}

	GString CharToStr(char c)
	{
		std::stringstream k;
		k << c;
		return k.str();
	}

	GString Format(GString str, ...)
	{
		char r[1024];
		int bfsize;
		va_list vl;
		va_start(vl,str);
		bfsize = vsnprintf(r, 1024, str.c_str(), vl);
		if (bfsize < 0)
		{
			Utility::DebugBreak();
			va_end(vl);
			return "";
		}

		vector<char> fmt(bfsize + 1);
		vsnprintf(&fmt[0], bfsize, str.c_str(), vl);
		va_end(vl);

		return GString(fmt.data());
	}

	vector<GString> TokenSplit(const GString& str, const GString &token, bool compress)
	{
		vector<GString> ret;
		size_t len = str.length();
		auto it = &str[0];
		auto next = strpbrk(str.c_str(), token.c_str()); // next token instance
		for (; next != nullptr; next = strpbrk(it, token.c_str()))
		{
			if (!compress || it - next != 0)
				ret.push_back(GString(it, next));
			it = next + 1;
		}

		if (it != next && len)
			ret.push_back(GString(it, &str[len]));
		return ret;
	}

	GString Trim(GString& str)
	{
		std::regex trimreg("^\\s*(.*?)\\s*$");
		return str = regex_replace(str, trimreg, "$1");
	}

	GString ReplaceAll(GString& str, const GString& seq, const GString what)
	{
		return str = regex_replace(str, std::regex(seq), what);
	}

	GString ToLower(GString& str)
	{
		transform(str.begin(), str.end(), str.begin(), ::tolower);
		return str;
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
		ReplaceAll(S, "[<>:\\|?*]", "");
		if (removeSlash)
			ReplaceAll(S, "/", "");
	}

	GString GetSha256ForFile(GString Filename)
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
