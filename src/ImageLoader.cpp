#include "Global.h"
#include <GL/glew.h>
#include <map>
#include <string>

#include "Image.h"
#include "ImageLoader.h"
#include "FileManager.h"

#include <boost/thread/mutex.hpp>
#include <SOIL.h>

boost::mutex LoadMutex;
std::map<String, Image*> ImageLoader::Textures;
std::map<String, ImageLoader::UploadData> ImageLoader::PendingUploads;

ImageLoader::ImageLoader()
{
}

ImageLoader::~ImageLoader()
{
	// unload ALL the images.
}

void ImageLoader::InvalidateAll()
{
	for (std::map<std::string, Image*>::iterator i = Textures.begin(); i != Textures.end(); i++)
	{
		i->second->IsValid = false;
	}
}

void ImageLoader::UnloadAll()
{
	for (std::map<std::string, Image*>::iterator i = Textures.begin(); i != Textures.end(); i++)
	{
		glDeleteTextures(1, &i->second->texture);
	}

	InvalidateAll();
}

GLuint ImageLoader::UploadToGPU(unsigned char* Data, unsigned int Width, unsigned int Height)
{
	GLuint texture;
	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &texture);


	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri( GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Width, Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, Data);

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

	if (glGenerateMipmap)
		glGenerateMipmap(GL_TEXTURE_2D);

	return texture;
}

Image* ImageLoader::InsertImage(String Name, unsigned int Texture, int Width, int Height)
{
	if (Textures.find(Name) == Textures.end())
	{
		Image* I = (Textures[Name] = new Image(Texture, Width, Height));
		I->IsValid = true;
		I->fname = Name;
		return I;
	}

	/* else */
	Textures[Name]->texture = Texture;
	Textures[Name]->w = Width;
	Textures[Name]->h = Height;
	Textures[Name]->IsValid = true;
	return Textures[Name];
}


Image* ImageLoader::Load(String filename)
{
	if ( Textures.find(filename) != Textures.end() && Textures[filename]->IsValid)
	{
		return Textures[filename];
	}
	else
	{	
		int width, height, channels;
		GLuint texture;
		unsigned char *image = SOIL_load_image(filename.c_str(), &width, &height, &channels,  SOIL_LOAD_RGBA);

		if (!image)
		{
			return NULL;
		}

		texture = UploadToGPU(image, width, height);

		SOIL_free_image_data(image);
		
		return InsertImage(filename, texture, width, height);
	}
	return 0;
}

Image* ImageLoader::LoadSkin(String filename)
{
	return Load(FileManager::GetSkinPrefix() + filename);
}

/* For multi-threaded loading. */
void ImageLoader::LoadFromManifest(char** Manifest, int Count, String Prefix)
{
	LoadMutex.lock();
	for (int i = 0; i < Count; i++)
	{
		UploadData New;
		int Channels;
		const char* FinalFilename = (Prefix + Manifest[i]).c_str();

		if (Textures.find(FinalFilename) == Textures.end())
		{
			New.Data = SOIL_load_image(FinalFilename, &New.Width, &New.Height, &Channels, SOIL_LOAD_RGBA);
			PendingUploads.insert( std::pair<char*, UploadData>((char*)FinalFilename, New) );
		}
	}
	LoadMutex.unlock();
}
	
	
void ImageLoader::UpdateTextures()
{
	if (PendingUploads.size() && LoadMutex.try_lock())
	{
		for (std::map<String, UploadData>::iterator i = PendingUploads.begin(); i != PendingUploads.end(); i++)
		{
			unsigned int Texture = UploadToGPU(i->second.Data, i->second.Width, i->second.Height);

			InsertImage(i->first, Texture, i->second.Width, i->second.Height);

			SOIL_free_image_data(i->second.Data);
		}

		PendingUploads.clear();
		LoadMutex.unlock();
	}

	if (Textures.size())
	{
		for (std::map<String, Image*>::iterator i = Textures.begin(); i != Textures.end(); i++)
		{
			if (i->second->IsValid) /* all of them are valid */
				break;

			Load(i->first);
		}
	}
}