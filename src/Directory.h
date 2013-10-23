#ifndef Directory_H
#define Directory_H

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
	~Directory();
	
	void operator=(std::string);
	bool operator==(std::string);

	Directory ParentDirectory();
	
	String path();
	const char* c_path();
	std::vector<String> *ListDirectory(std::vector<String> *Vec = NULL, DirType T = FS_REG, const char* ext = NULL, bool Recursive = false);
	
private:
	String curpath;

};

extern Directory operator/(Directory, String);
extern Directory operator/(String, Directory);

#endif