#include <filesystem>
#include <string>
#include <locale>
#include <codecvt>

#ifdef LINUX
#include <iconv.h>
#endif

#ifdef WIN32
#include <windows.h>
#endif

#define MAX_STRING_SIZE 8192

namespace Conversion {
std::string SJIStoU8(std::string Line)
    {
#ifdef WIN32
        wchar_t u16s[MAX_STRING_SIZE];
        char mbs[MAX_STRING_SIZE];
        size_t len = MultiByteToWideChar(932, 0, Line.c_str(), Line.length(), u16s, MAX_STRING_SIZE);
        len = WideCharToMultiByte(CP_UTF8, 0, u16s, len, mbs, MAX_STRING_SIZE, NULL, NULL);
        mbs[len] = 0;
        return std::string(mbs);
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
        return std::string(buf);
#else
        char buf[MAX_STRING_SIZE];
        iconv_t conv;
        char* out = buf;
        const char* in = Line.c_str();
        size_t BytesLeftSrc = Line.length();
        size_t BytesLeftDst = MAX_STRING_SIZE;

        conv = iconv_open("UTF-8", "SHIFT_JIS");
        bool success = (iconv(conv, (char **)&in, &BytesLeftSrc, &out, &BytesLeftDst) > -1);

        iconv_close(conv);
        if (success)
            return std::string(out);
        else
        {
            // Log::Printf("Failure converting character sets.");
            return std::string();
        }
#endif
    }

std::wstring Widen(std::string Line)
    {
		std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        return converter.from_bytes(Line);
    }

    std::string ToU8(std::wstring Line)
    {
		std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        return converter.to_bytes(Line);
    }

	std::string ToLocaleStr(std::wstring Line) {
#ifdef WIN32
        char mbs[MAX_STRING_SIZE];
        size_t len = WideCharToMultiByte(0, 0, Line.c_str(), -1, mbs, MAX_STRING_SIZE, NULL, 0);
        mbs[len] = 0;
        return std::string(mbs);
#else
		return ToU8(Line);
#endif
	}

	std::wstring FromLocaleStr(std::string s)
	{
#ifdef WIN32
		wchar_t wcs[MAX_STRING_SIZE];
		size_t len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), s.length(), wcs, MAX_STRING_SIZE);
		wcs[len] = 0;
		return std::wstring(wcs);
#else
		return Widen(s);
#endif
	}

	std::string LocaleToU8(std::string line)
	{
		return ToU8(FromLocaleStr(line));
	}

}