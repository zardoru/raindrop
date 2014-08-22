#ifdef _WIN32
#include <Windows.h>
#else
#include <dirent.h>
#endif

#include "Global.h"
#include "Directory.h"

Directory::Directory()
{
	
}

Directory::~Directory()
{
	
}

Directory::Directory(std::string subpath)
{
	curpath = subpath;
}

Directory::Directory(const char* path)
{
	curpath = path;
}

void Directory::operator=(std::string subpath)
{
	curpath = subpath;
}

bool Directory::operator==(std::string subpath) const
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
	return Directory(std::string(".")); // if there is no slash, then the root directory has been reached.
}

Directory operator/(Directory parent, std::string subpath)
{
	if(subpath == "..") return parent.ParentDirectory();

	std::string newpath;

	char last = parent.path()[parent.path().length()-1];
		
	if (last == '/' || last == '\\')
		newpath = parent.path() + subpath;
	else
		newpath = parent.path() + "/" + subpath;

	return Directory(newpath);
}

Directory operator/(std::string subpath, Directory parent)
{
	return operator/( Directory(subpath), std::string(parent)); // o_O
}

Directory Directory::Filename()
{
	size_t place;
	if ((place = curpath.find_last_of('/')) != std::string::npos)
	{
		return curpath.substr(place+1);
	}else if ((place = curpath.find_last_of('\\')) != std::string::npos)
	{
		return curpath.substr(place+1);
	}

	return curpath;
}

String Directory::GetExtension() const
{
	return curpath.substr(curpath.find_last_of(".")+1);
}

std::string Directory::path() const { return curpath; }
const char* Directory::c_path() const { return curpath.c_str(); }

std::vector<std::string>& Directory::ListDirectory(std::vector<std::string>& Vec, DirType T, const char* ext, bool Recursive)
{

#ifdef _WIN32
	HANDLE hFind = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATA ffd;
	std::string DirFind = curpath + "/*";
	wchar_t tmp[MAX_PATH];
	char tmpmbs[MAX_PATH];

	memset(tmp, 0, sizeof(wchar_t) * MAX_PATH);

	std::wstring Wide = Utility::Widen(curpath) + L"/*";

	hFind = FindFirstFile((LPCWSTR)Wide.c_str(), &ffd);

	do {

		if (hFind == INVALID_HANDLE_VALUE)
			continue;

		if ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY && T == FS_DIR) || (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && T == FS_REG))
		{
			std::wstring fname = ffd.cFileName;

			if (fname == L"." || fname == L"..")
				continue;

			if (ext)
			{
				mbstowcs(tmp, ext, MAX_PATH);
				if (fname.substr(fname.find_last_of(L".")+1) == std::wstring(tmp)) 
				{
					Vec.push_back(Utility::Narrow(fname));
				}
			}

			if (!ext)
			{
				Vec.push_back(Utility::Narrow(fname));
			}

		}else
		{
			wcstombs(tmpmbs, ffd.cFileName, MAX_PATH);

			std::string fname = curpath + tmpmbs + "./*";

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
			if (dir->d_type == DT_REG || dir->d_type == DT_DIR && T == FS_DIR)
			{
				std::string fname = dir->d_name;
				
				// Extension is what we need?
				if (ext && fname.substr(fname.find_last_of(".")+1) == std::string(ext)) 
					Vec.push_back(fname);
				else if (!ext)
					Vec.push_back(fname);

			}else if (dir->d_type == DT_DIR)
			{
				std::string fname = std::string(dir->d_name) + "/";

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

Directory::operator std::string() const
{
	return path();
}