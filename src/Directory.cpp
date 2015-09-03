#ifdef _WIN32
#include <Windows.h>
#else
#include <dirent.h>
#endif

#include "Global.h"
#include "Directory.h"

#include <boost/algorithm/string.hpp>

Directory::Directory()
{
	
}

Directory::~Directory()
{
	
}

Directory::Directory(GString subpath)
{
	curpath = subpath;
}

Directory::Directory(const char* path)
{
	curpath = path;
}

void Directory::operator=(GString subpath)
{
	curpath = subpath;
}

bool Directory::operator==(GString subpath) const
{
	return curpath == subpath;
}

Directory Directory::ParentDirectory()
{
	int a = curpath.length();
	while(--a){
		if(curpath[a] == '/' || curpath[a] == '\\'){
			return Directory(curpath.substr(0, a));
		}
	}
	return Directory(GString(".")); // if there is no slash, then the root directory has been reached.
}

void Directory::Normalize(bool RemoveIllegal)
{
	if (RemoveIllegal) {
		boost::replace_all(curpath, "<", "");
		boost::replace_all(curpath, ">", "");
		boost::replace_all(curpath, ":", "");
		boost::replace_all(curpath, "\"", "");
		boost::replace_all(curpath, "|", "");
		boost::replace_all(curpath, "?", "");
		boost::replace_all(curpath, "*", "");
		boost::replace_all(curpath, "\\", "/");
	}

	GString newCurPath;

	// remove all redundant slashes
	char last = 0;
	for (auto i = curpath.begin(); i != curpath.end(); ++i)
	{
		if (last == '/' && *i == '/')
		{
			continue;
		}

		last = *i;
		newCurPath += *i;
	}

	curpath = newCurPath;
}

Directory operator/(Directory parent, GString subpath)
{
	if(subpath == "..") return parent.ParentDirectory();

	GString newpath;

	if (!parent.path().length())
		return subpath;

	char last = parent.path()[parent.path().length()-1];
		
	if (last == '/' || last == '\\')
		newpath = parent.path() + subpath;
	else
		newpath = parent.path() + "/" + subpath;

	return Directory(newpath);
}

Directory operator/(GString subpath, Directory parent)
{
	return operator/( Directory(subpath), GString(parent)); // o_O
}

Directory Directory::Filename()
{
	size_t place;
	if ((place = curpath.find_last_of('/')) != GString::npos)
	{
		return curpath.substr(place+1);
	}else if ((place = curpath.find_last_of('\\')) != GString::npos)
	{
		return curpath.substr(place+1);
	}

	return curpath;
}

GString Directory::GetExtension() const
{
	auto out = curpath.substr(curpath.find_last_of(".")+1);
	boost::to_lower(out);
	return out;
}

GString Directory::path() const { return curpath; }
const char* Directory::c_path() const { return curpath.c_str(); }

vector<GString>& Directory::ListDirectory(vector<GString>& Vec, DirType T, const char* ext, bool Recursive)
{

#ifdef _WIN32
	HANDLE hFind = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATA ffd;
	GString DirFind = curpath + "/*";
	wchar_t tmp[MAX_PATH];
	char tmpmbs[MAX_PATH];

	memset(tmp, 0, sizeof(wchar_t) * MAX_PATH);

	std::wstring Wide;
	
	if (ext == nullptr)
		Wide = Utility::Widen(curpath) + L"/*";
	else
		Wide = Utility::Widen(curpath) + L"/*." + Utility::Widen(ext);

	hFind = FindFirstFile(LPCWSTR(Wide.c_str()), &ffd);

	do {

		if (hFind == INVALID_HANDLE_VALUE)
			continue;

		if ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY && T == FS_DIR) || (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && T == FS_REG))
		{
			std::wstring fname = ffd.cFileName;

			if (fname == L"." || fname == L"..")
				continue;

			Vec.push_back(Utility::Narrow(fname));
		}else
		{
			wcstombs(tmpmbs, ffd.cFileName, MAX_PATH);

			GString fname = curpath + tmpmbs + "./*";

			if (Recursive)
			{
				Directory NewPath (fname);
				NewPath.ListDirectory(Vec, T, ext, Recursive);
			}
		}
	}while (FindNextFile(hFind, &ffd) != 0);

	FindClose(hFind);

#else // Unixlike?
	DIR           *d;
	struct dirent *dir;
	d = opendir(curpath.c_str());
	if (d)
	{
		while ((dir = readdir(d)) != NULL)
		{
			if (dir->d_type == DT_REG || (dir->d_type == DT_DIR && T == FS_DIR))
			{
				GString fname = dir->d_name;
				
				// Extension is what we need?
				if (ext && fname.substr(fname.find_last_of(".")+1) == GString(ext)) 
					Vec.push_back(fname);
				else if (!ext)
					Vec.push_back(fname);

			}else if (dir->d_type == DT_DIR)
			{
				GString fname = GString(dir->d_name) + "/";

				if (Recursive)
				{
					Directory NewPath (fname);
					NewPath.ListDirectory(Vec, T, ext, Recursive);
				}
			}
		}

		closedir(d);
	}
#endif
	return Vec;
}

Directory::operator GString() const
{
	return path();
}