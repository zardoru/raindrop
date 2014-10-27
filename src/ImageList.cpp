#include <map>

#include "Global.h"
#include "GameState.h"
#include "Image.h"
#include "ImageList.h"
#include "ImageLoader.h"
#include "GraphObject2D.h"

#include <boost/algorithm/string/replace.hpp>

ImageList::ImageList(bool ReleaseAtDestruction)
{
	ShouldDeleteAtDestruction = ReleaseAtDestruction;
}

ImageList::~ImageList()
{
	if (!ShouldDeleteAtDestruction)
		return;

	Destroy();
}

void ImageList::AddToList(const GString Filename, const GString Prefix)
{
	GString ResFilename = Directory(Prefix) / Filename;

	boost::replace_all (ResFilename, "//", "/");

	if (Images.find(ResFilename) == Images.end())
	{
		ImageLoader::AddToPending(ResFilename.c_str());
		Images[ResFilename] = NULL;
	}
}

void ImageList::AddToListIndex(const GString Filename, const GString Prefix, int Index)
{
	GString ResFilename = Directory(Prefix) / Filename;

	boost::replace_all (ResFilename, "//", "/");

	if (ImagesIndex.find(Index) == ImagesIndex.end())
	{
		ImageLoader::AddToPending(ResFilename.c_str());
		Images[ResFilename] = NULL;
		ImagesIndex[Index] = NULL;
		ImagesIndexPending[Index] = ResFilename;
	}

}

void ImageList::Destroy()
{
	for (std::map<GString, Image*>::iterator i = Images.begin(); i != Images.end(); i++)
		ImageLoader::DeleteImage(i->second);
}

void ImageList::AddToList(const uint32 Count, const GString *Filename, const GString Prefix)
{
	for (uint32 i = 0; i < Count; i++)
	{
		AddToList(Filename[i], Prefix);
	}
}

bool ImageList::LoadAll()
{
	bool WereErrors = false;
	for (std::map<GString, Image*>::iterator i = Images.begin(); i != Images.end(); i++)
	{
		i->second = ImageLoader::Load(i->first);
		if (i->second == NULL)
			WereErrors = true;
	}

	for (std::map<int, GString>::iterator i = ImagesIndexPending.begin(); i != ImagesIndexPending.end(); i++)
	{
		ImagesIndex[i->first] = ImageLoader::Load(i->second);
		if (ImagesIndex[i->first] == NULL)
			WereErrors = true;
	}

	ImagesIndexPending.clear();

	return WereErrors;
}

// Gets image from this filename
Image* ImageList::GetFromFilename(const GString Filename)
{
	return Images[Filename];
}

// Gets image from SkinPrefix + filename
Image* ImageList::GetFromSkin(const GString Filename)
{
	return Images[GameState::GetInstance().GetSkinPrefix() + Filename];
}

Image* ImageList::GetFromIndex(int Index)
{
	return ImagesIndex[Index];
}

void ImageList::ForceFetch()
{
	GraphObject2D Fill;

	// Draw as black.
	Fill.Red = Fill.Blue = Fill.Green = 0;
	Fill.Alpha = 0.0001f;

	for (std::map<GString, Image*>::iterator i = Images.begin(); i != Images.end(); i++)
	{
		Fill.SetImage (i->second);
		Fill.Render();
	}
}