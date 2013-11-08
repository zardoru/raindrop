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
		if(curpath[a] == '/'){
			return Directory(curpath.substr(0, a));
		}
	}
	return Directory(std::string(".")); // if there is no slash, then the root directory has been reached.
}

Directory operator/(Directory parent, std::string subpath)
{
	if(subpath == "..") return parent.ParentDirectory();

	std::string newpath = parent.path() + "/" + subpath;
	Directory newdir(newpath);
	return newdir;
}

Directory operator/(std::string subpath, Directory parent)
{
	return operator/(parent, subpath); // o_O
}

std::string Directory::path() const { return curpath; }
const char* Directory::c_path() const { return curpath.c_str(); }

std::vector<std::string> *Directory::ListDirectory(std::vector<std::string> *Vec, DirType T, const char* ext, bool Recursive)
{
	std::vector<std::string> *Out = NULL;
	
	if (!Vec)
		Out = new std::vector<std::string>();
	else
		Out = Vec;

#ifdef _WIN32
	HANDLE hFind = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATA ffd;
	std::string DirFind = curpath + "/*";

	hFind = FindFirstFile(DirFind.c_str(), &ffd);

	if (hFind == INVALID_HANDLE_VALUE)
		return Out;

	do {
		if ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY && T == FS_DIR) || (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && T == FS_REG))
		{
			std::string fname = ffd.cFileName;

			if (ext && fname.substr(fname.find_last_of(".")+1) == std::string(ext)) 
				Out->push_back(fname);
			if (!ext)
				Out->push_back(fname);
		}else
		{
			std::string fname = curpath + ffd.cFileName + "/*";

			if (Recursive)
			{
				Directory NewPath (fname);
				NewPath.ListDirectory(Out, T, ext, Recursive);
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
			if (dir->d_type == DT_REG)
			{
				std::string fname = dir->d_name;
				
				// Extension is what we need?
				if (ext && fname.substr(fname.find_last_of(".")+1) == std::string(ext)) 
					Out->push_back(fname);
				else if (!ext)
					Out->push_back(fname);
			}else if (dir->d_type == DT_DIR)
			{
				std::string fname = curpath + "/" + dir->d_name + "/";

				if (Recursive)
				{
					Directory NewPath (fname);
					NewPath.ListFiles(Out, T, ext, Recursive);
				}
		}

		closedir(d);
	}
#endif
	return Out;
}