namespace Utility
{
    void DebugBreak();
    bool IsNumeric(const char* s);	

    int GetLastModifiedTime(std::filesystem::path Path);
    std::string GetSha256ForFile(std::filesystem::path Filename);
    
    void RemoveFilenameIllegalCharacters(std::string &S, bool removeSlash, bool noAbsolute = true);

    std::string Format(std::string str, ...);

    std::vector<std::string> TokenSplit(const std::string &str, const std::string &token = ",", bool compress = false);

    std::string Trim(std::string& str);
    std::string ReplaceAll(std::string& str, const std::string& seq, const std::string what);
    std::string ToLower(std::string& str); // Caveat: only for ascii purposes.

	std::vector<std::filesystem::path> GetFileListing(std::filesystem::path path);

    template <class T>
    std::string Join(const T& iterable, const std::string& seq)
    {
        std::string ret;
        for (auto s = iterable.begin(); s != iterable.end(); ++s)
        {
            auto next = s; ++next;
            if (next != iterable.end())
                ret += *s + seq;
            else
                ret += *s;
        }

        return ret;
    }
}

namespace Conversion {
    // Convert utf8 string into std::wstring.
    std::wstring Widen(std::string Line);

	// Convert system locale string into std::wstring.
	std::wstring FromLocaleStr(std::string Line);

	// Convert system locale string into U8.
	std::string LocaleToU8(std::string line);

	// Convert std::wstring into utf8 std::string.
    std::string ToU8(std::wstring Line);

	// Convert std::wstring into system locale std::string
	std::string ToLocaleStr(std::wstring Line);

	// Convert SHIFT-JIS std::string into UTF-8 std::string.
    std::string SJIStoU8(std::string Line);
}

std::string IntToStr(int num);
std::string CharToStr(char c);


int b36toi(const char* txt);
int b16toi(const char* txt);

template <class F, class T>
T filter(F pred, const T &ctr)
{
	T rt_val;
	for (auto&& v: ctr)
	{
		if (pred(v))
			rt_val.insert(rt_val.cend(), v);
	}

	return rt_val;
}
