

// A front-end to ImageLoader that unloads all images added to the list on destruction.

/*
	In particular, allows a manifest of filenames to be passed to it and control when it loads those images.
*/
class ImageList {

	std::map <GString, Image*> Images;

	std::map <int, GString> ImagesIndexPending;
	std::map <int, Image*> ImagesIndex;
	bool ShouldDeleteAtDestruction;

public:

	ImageList(bool ReleaseAtDestruction = true);
	~ImageList();

	void Destroy();
	void AddToList(const GString Filename, const GString Prefix);
	void AddToListIndex(const GString Filename, const GString Prefix, int Index);
	void AddToList(const uint32 Count, const GString *Filename, const GString Prefix);
	bool LoadAll();

	void ForceFetch();

	// Gets image from this filename
	Image* GetFromFilename(const GString Filename);

	// Gets image from this index
	Image* GetFromIndex(int Index);

	// Gets image from SkinPrefix + filename
	Image* GetFromSkin(const GString Filename);
};