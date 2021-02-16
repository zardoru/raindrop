#include <cstdlib>
#include <string>
#include <vector>
#include <cstdio>
#include <regex>
#include <csignal>
#include <fstream>
#include <cstdarg>
#include <sstream>
#include <filesystem>


#include "TextAndFileUtil.h"

#include "sha256/sha256.h"

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
        __debugbreak();
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
            {
                ret.push_back(str.substr(it - str.c_str(), next - it));
            }
            it = next + 1;
        }

        if (it != next && len)
        {
			ret.push_back(str.substr(it - str.c_str(), next - it));
        }
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
        std::transform(str.begin(), str.end(), str.begin(), ::tolower);
        return str;
    }

    time_t GetLastModifiedTime(std::filesystem::path Path)
    {
		if (std::filesystem::exists(Path)) {
#ifndef WIN32
			auto a = std::filesystem::last_write_time(Path);
            return decltype(a)::clock::to_time_t(a);
#else
            // az: bah. fucking windows.
            struct _stat s{};
            _wstat(Path.c_str(), &s);
            return s.st_mtime;
#endif
		}
		else return -1;
    }

    void RemoveFilenameIllegalCharacters(std::string &S, bool removeSlash, bool noAbsolute)
    {
        // size_t len = strlen(fn);
        /*if (!noAbsolute)
            ReplaceAll(S, "[<>\\|?*]", "");
        else*/
            ReplaceAll(S, "[<>\\|?*]", "");

        if (removeSlash)
            ReplaceAll(S, "/", "");
    }

    std::string GetSha256ForFile(std::filesystem::path Filename)
    {
        SHA256 SHA;
		std::ifstream InStream(Filename, std::ios::in | std::ios::binary);
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

		if (!std::filesystem::exists(path)) return out;
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