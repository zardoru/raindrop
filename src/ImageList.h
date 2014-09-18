

// A front-end to ImageLoader that unloads all images added to the list on destruction.

/*
	In particular, allows a manifest of filenames to be passed to it and control when it loads those images.
*/
class ImageList {

	std::map <String, Image*> Images;

	std::map <int, String> ImagesIndexPending;
	std::map <int, Image*> ImagesIndex;
	bool ShouldDeleteAtDestruction;

public:

	ImageList(bool ReleaseAtDestruction = true);
	~ImageList();

	void Destroy();
	void AddToList(const String Filename, const String Prefix);
	void AddToListIndex(const String Filename, const String Prefix, int Index);
	void AddToList(const uint32 Count, const String *Filename, const String Prefix);
	bool LoadAll();

	void ForceFetch();

	// Gets image from this filename
	Image* GetFromFilename(const String Filename);

	// Gets image from this index
	Image* GetFromIndex(int Index);

	// Gets image from SkinPrefix + filename
	Image* GetFromSkin(const String Filename);
};