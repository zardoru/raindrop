#include "Global.h"
#include <GL/glew.h>
#include <map>
#include <string>

#include "Image.h"
#include "ImageLoader.h"
#include "FileManager.h"

#include <SOIL.h>


std::map<String, Image*> ImageLoader::Textures;

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
		
		GLenum Var = glGetError();

		glActiveTexture(GL_TEXTURE0);
		glGenTextures(1, &texture);

			
		glBindTexture(GL_TEXTURE_2D, texture);

		glTexParameteri( GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );

		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

		if (glGenerateMipmap)
			glGenerateMipmap(GL_TEXTURE_2D);

		Var = glGetError();
		
		if (Var != GL_NO_ERROR)
		{
			Utility::DebugBreak();
		}

		SOIL_free_image_data(image);
		
		if (Textures.find(filename) == Textures.end())
		{
			Image* I = (Textures[filename] = new Image(texture, width, height));
			I->IsValid = true;
			I->fname = filename;
			return I;
		}else
		{
			Textures[filename]->texture = texture;
			Textures[filename]->w = width;
			Textures[filename]->h = height;
			Textures[filename]->IsValid = true;
			return Textures[filename];
		}
	}
	return 0;
}

Image* ImageLoader::LoadSkin(String filename)
{
	return Load(FileManager::GetSkinPrefix() + filename);
}