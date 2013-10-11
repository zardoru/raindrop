#include "Global.h"
#include <GL/glew.h>
#include <map>
#include <string>

#include "Image.h"
#include "ImageLoader.h"
#include "FileManager.h"

#include <SOIL.h>


std::map<std::string, Image*> ImageLoader::Textures;

ImageLoader::ImageLoader()
{
}

ImageLoader::~ImageLoader()
{
	// unload ALL the images.
}

Image* ImageLoader::Load(std::string filename)
{
	if ( Textures.find(filename) != Textures.end() )
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
		// reaching this point is unlikely.
		return (Textures[filename] = new Image(texture, width, height));
	}
	return 0;
}

Image* ImageLoader::LoadSkin(std::string filename)
{
	return Load(FileManager::GetSkinPrefix() + filename);
}