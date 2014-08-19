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

void ImageList::AddToList(const String Filename, const String Prefix)
{
	String ResFilename = Prefix + "/" + Filename;

	boost::replace_all (ResFilename, "//", "/");

	if (Images.find(ResFilename) == Images.end())
	{
		ImageLoader::AddToPending(ResFilename.c_str());
		Images[ResFilename] = NULL;
	}
}

void ImageList::Destroy()
{
	for (std::map<String, Image*>::iterator i = Images.begin(); i != Images.end(); i++)
		ImageLoader::DeleteImage(i->second);
}

void ImageList::AddToList(const uint32 Count, const String *Filename, const String Prefix)
{
	for (uint32 i = 0; i < Count; i++)
	{
		AddToList(Filename[i], Prefix);
	}
}

bool ImageList::LoadAll()
{
	bool WereErrors = false;
	for (std::map<String, Image*>::iterator i = Images.begin(); i != Images.end(); i++)
	{
		i->second = ImageLoader::Load(i->first);
		if (i->second == NULL)
			WereErrors = true;
	}

	return WereErrors;
}

// Gets image from this filename
Image* ImageList::GetFromFilename(const String Filename)
{
	return Images[Filename];
}

// Gets image from SkinPrefix + filename
Image* ImageList::GetFromSkin(const String Filename)
{
	return Images[GameState::GetInstance().GetSkinPrefix() + Filename];
}

void ImageList::ForceFetch()
{
	GraphObject2D Fill;

	// Draw as black.
	Fill.Red = Fill.Blue = Fill.Green = 0;
	Fill.Alpha = 0.0001;

	for (std::map<String, Image*>::iterator i = Images.begin(); i != Images.end(); i++)
	{
		Fill.SetImage (i->second);
		Fill.Render();
	}
}