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
	Directory(std::string path);
	Directory(const char* path);
	~Directory();
	
	void operator=(std::string);
	bool operator==(std::string) const;
	operator std::string() const;


	void Normalize(bool RemoveIllegal = false);
	Directory ParentDirectory();
	Directory Filename();
	
	std::string GetExtension() const;
	std::string path() const;
	const char* c_path() const;
	std::vector<std::string>& ListDirectory(std::vector<std::string>& Vec, DirType T = FS_REG, const char* ext = NULL, bool Recursive = false);
	
private:
	std::string curpath;

};

extern Directory operator/(Directory, std::string);
extern Directory operator/(std::string, Directory);

//#endif