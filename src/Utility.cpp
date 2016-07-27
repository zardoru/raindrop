#include "pch.h"

#include "Logging.h"

int b36toi(const char *txt)
{
	return strtoul(txt, nullptr, 36);
}

int b16toi(const char *txt)
{
	return strtoul(txt, nullptr, 16);
}

namespace Utility
{
    const short MAX_STRING_SIZE = 2048;

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
#else
		return Widen(s);
#endif
	}

	std::string LocaleToU8(std::string line)
	{
		return ToU8(FromLocaleStr(line));
	}


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
            Log::Printf("Failure converting character sets.");
            return std::string();
        }
#endif
    }

    std::string IntToStr(int num)
    {
        std::stringstream k;
        k << num;
        return k.str();
    }

    std::string CharToStr(char c)
    {
        std::stringstream k;
        k << c;
        return k.str();
    }

    std::string Format(std::string str, ...)
    {
        char r[1024];
        int bfsize;
        va_list vl;
        va_start(vl, str);
        bfsize = vsnprintf(r, 1024, str.c_str(), vl);
        if (bfsize < 0)
        {
            Utility::DebugBreak();
            va_end(vl);
            return "";
        }

        std::vector<char> fmt(bfsize + 1);
        vsnprintf(&fmt[0], bfsize + 1, str.c_str(), vl);
        va_end(vl);

        return std::string(fmt.data());
    }

    std::vector<std::string> TokenSplit(const std::string& str, const std::string &token, bool compress)
    {
        std::vector<std::string> ret;
        size_t len = str.length();
        auto it = &str[0];
        auto next = strpbrk(str.c_str(), token.c_str()); // next token instance
        for (; next != nullptr; next = strpbrk(it, token.c_str()))
        {
            if (!compress || it - next != 0)
                ret.push_back(std::string(it, next));
            it = next + 1;
        }

        if (it != next && len)
            ret.push_back(std::string(it, &str[len]));
        return ret;
    }

    std::string Trim(std::string& str)
    {
        std::regex trimreg("^\\s*(.*?)\\s*$");
        return str = regex_replace(str, trimreg, "$1");
    }

    std::string ReplaceAll(std::string& str, const std::string& seq, const std::string what)
    {
        return str = regex_replace(str, std::regex(seq), what);
    }

    std::string ToLower(std::string& str)
    {
        transform(str.begin(), str.end(), str.begin(), ::tolower);
        return str;
    }

    void CheckDir(std::string path)
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

    int GetLMT(std::filesystem::path Path)
    {
		if (std::filesystem::exists(Path)) {
			auto a = std::filesystem::last_write_time(Path);
			return decltype(a)::clock::to_time_t(a);
		}
		else return -1;
    }

    void RemoveFilenameIllegalCharacters(std::string &S, bool removeSlash, bool noAbsolute)
    {
        // size_t len = strlen(fn);
        if (!noAbsolute)
            ReplaceAll(S, "[<>\\|?*]", "");
        else
            ReplaceAll(S, "[<>\\|?*]", "");

        if (removeSlash)
            ReplaceAll(S, "/", "");
    }

    std::string GetSha256ForFile(std::filesystem::path Filename)
    {
        SHA256 SHA;
        std::ifstream InStream(Filename.string());
        unsigned char tmpbuf[256];

        if (!InStream.is_open())
            return "";

        while (!InStream.eof())
        {
            InStream.read((char*)tmpbuf, 256);
            size_t cnt = InStream.gcount();

            SHA.add(tmpbuf, cnt);
        }

        return std::string(SHA.getHash());
    }

	std::vector<std::filesystem::path> GetFileListing(std::filesystem::path path)
	{
		std::vector<std::filesystem::path> out;
		for (auto &p : std::filesystem::directory_iterator(path)) {
			out.push_back(p);
		}

		return out;
	}
} // namespace Utility

double latof(std::string s)
{
    char point = *localeconv()->decimal_point;

    if (s.find_first_of(point) == std::string::npos)
    {
        char toFind = '.';
        if (point == ',') toFind = '.';
        else if (point == '.') toFind = ',';

        size_t idx = s.find_first_of(toFind);
        if (idx != std::string::npos)
            s[idx] = point;
    }

    return atof(s.c_str());
}
