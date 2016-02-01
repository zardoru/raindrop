//#if 0

#pragma once

class Directory
{

public:

	enum DirType
	{
		FS_DIR,
		FS_REG
	};

	Directory();
	Directory(GString path);
	Directory(const char* path);
	~Directory();
	
	void operator=(GString);
	bool operator==(GString) const;
	operator GString() const;


	void Normalize(bool RemoveIllegal = false);
	Directory ParentDirectory();
	Directory Filename();
	
	GString GetExtension() const;
	GString path() const;
	const char* c_path() const;
	std::vector<GString>& ListDirectory(std::vector<GString>& Vec, DirType T = FS_REG, const char* ext = NULL, bool Recursive = false);
	
private:
	GString curpath;

};

extern Directory operator/(Directory, GString);
extern Directory operator/(GString, Directory);

//#endif