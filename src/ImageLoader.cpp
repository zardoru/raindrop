#ifdef WIN32
#include <windows.h>
#endif

#include "Global.h"
#include "Logging.h"
#include <GL/glew.h>
#include <map>

#include "Image.h"
#include "ImageLoader.h"

#include <boost/thread/mutex.hpp>
#include <SOIL/SOIL.h>

boost::mutex LoadMutex;
std::map<GString, Image*> ImageLoader::Textures;
std::map<GString, ImageLoader::UploadData> ImageLoader::PendingUploads;

void Image::CreateTexture()
{
	if (texture == -1 || !IsValid)
	{
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);

		LastBound = this;
		IsValid = true;
	}
}

void Image::BindNull()
{
	glBindTexture(GL_TEXTURE_2D, 0);
	LastBound = NULL;
}

void Image::Bind()
{
	if (IsValid && texture != -1)
	{
		if (LastBound != this)
		{
			glBindTexture(GL_TEXTURE_2D, texture);
			LastBound = this;
		}
	}
}

void Image::Destroy() // Called at destruction time
{
	if (IsValid && texture != -1)
	{
		glDeleteTextures(1, &texture);
		IsValid = false;
		texture = -1;
	}
}

void Image::SetTextureData(ImageData *Data, bool Reassign)
{
	if (Reassign) Destroy();

	CreateTexture(); // Make sure our texture exists.
	Bind();

	if (Data->Data == NULL && !Reassign)
	{
		return;
	}

	if (!TextureAssigned || Reassign) // We haven't set any data to this texture yet, or we want to regenerate storage
	{
		TextureAssigned = true;

		glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);

		GLint param;
		switch (Data->WrapMode)
		{
		case ImageData::WM_CLAMP_TO_EDGE:
			param = GL_CLAMP_TO_EDGE;
			break;
		case ImageData::WM_REPEAT:
			param = GL_REPEAT;
			break;
		default:
			param = GL_CLAMP_TO_EDGE;
		}

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, param);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, param);

		GLint minparam;
		switch (Data->ScalingMode)
		{
		case ImageData::SM_LINEAR:
			minparam = GL_LINEAR;
			param = GL_LINEAR;
			break;
		case ImageData::SM_MIPMAP:
			minparam = GL_LINEAR_MIPMAP_LINEAR;
			param = GL_LINEAR;
			break;
		case ImageData::SM_NEAREST:
			minparam = GL_NEAREST;
			param = GL_NEAREST;
			break;
		default:
			minparam = GL_LINEAR_MIPMAP_LINEAR;
			param = GL_LINEAR;
		}

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minparam);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, param);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Data->Width, Data->Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, Data->Data);
	}
	else // We did, so let's update instead.
	{
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, Data->Width, Data->Height, GL_RGBA, GL_UNSIGNED_BYTE, Data->Data);
	}

	w = Data->Width;
	h = Data->Height;
}

void Image::Assign(Directory Filename, ImageData::EScalingMode ScaleMode ,
	ImageData::EWrapMode WrapMode, 
	bool Regenerate)
{
	ImageData Ret;
	CreateTexture();

	Ret = ImageLoader::GetDataForImage(Filename);
	Ret.ScalingMode = ScaleMode;
	Ret.WrapMode = WrapMode;
	SetTextureData(&Ret, Regenerate);
	fname = Filename;
	free(Ret.Data);
}

ImageLoader::ImageLoader()
{
}

ImageLoader::~ImageLoader()
{
	// unload ALL the images.
}

void ImageLoader::InvalidateAll()
{
	for (std::map<GString, Image*>::iterator i = Textures.begin(); i != Textures.end(); i++)
	{
		i->second->IsValid = false;
	}
}

void ImageLoader::UnloadAll()
{
	InvalidateAll();

	for (std::map<GString, Image*>::iterator i = Textures.begin(); i != Textures.end(); i++)
	{
		glDeleteTextures(1, &i->second->texture);
	}
}

void ImageLoader::DeleteImage(Image* &ToDelete)
{
	if (ToDelete)
	{
		Textures.erase(Textures.find(ToDelete->fname));
		delete ToDelete;
		ToDelete = NULL;
	}
}

Image* ImageLoader::InsertImage(GString Name, ImageData *imgData)
{
	Image* I;

	if (!imgData || imgData->Data == NULL) return NULL;
	
	if (Textures.find(Name) == Textures.end())
		I = (Textures[Name] = new Image());
	else
		I = Textures[Name];

	I->SetTextureData(imgData);
	I->fname = Name;
	
	return I;
}

ImageData ImageLoader::GetDataForImage(GString filename)
{
	ImageData out;
	int channels;
	out.Data = SOIL_load_image(filename.c_str(), &out.Width, &out.Height, &channels, SOIL_LOAD_RGBA);

	if (!out.Data)
	{
		Log::Printf("Could not load image \"%s\". (%s)\n", filename.c_str(), SOIL_last_result());
	}

	return out;
}

ImageData ImageLoader::GetDataForImageFromMemory(const unsigned char* const buffer, size_t len)
{
	ImageData out;
	int channels;
	out.Data = SOIL_load_image_from_memory(buffer, len, &out.Width, &out.Height, &channels, SOIL_LOAD_RGBA);

	if (!out.Data)
		Log::Printf("Could not load image from memory: %s\n", SOIL_last_result());

	return out;
}


Image* ImageLoader::Load(GString filename)
{
	if ( Textures.find(filename) != Textures.end() && Textures[filename]->IsValid)
	{
		return Textures[filename];
	}
	else
	{	
		ImageData ImgData = GetDataForImage(filename);
		Image* Ret = InsertImage(filename, &ImgData);

		free(ImgData.Data);
		Image::LastBound = Ret;

		return Ret;
	}
	return 0;
}

void ImageLoader::AddToPending(const char* Filename)
{
	UploadData New;
	int Channels;
	if (Textures.find(Filename) == Textures.end())
	{
		New.Data = SOIL_load_image(Filename, &New.Width, &New.Height, &Channels, SOIL_LOAD_RGBA);
		LoadMutex.lock();
		PendingUploads.insert( std::pair<char*, UploadData>((char*)Filename, New) );
		LoadMutex.unlock();
	}
}

/* For multi-threaded loading. */
void ImageLoader::LoadFromManifest(char** Manifest, int Count, GString Prefix)
{
	for (int i = 0; i < Count; i++)
	{
		const char* FinalFilename = (Prefix + Manifest[i]).c_str();
		AddToPending(FinalFilename);
	}
}
	
	
void ImageLoader::UpdateTextures()
{
	if (PendingUploads.size() && LoadMutex.try_lock())
	{
		for (std::map<GString, UploadData>::iterator i = PendingUploads.begin(); i != PendingUploads.end(); i++)
		{
			ImageData imgData;
			imgData.Data = i->second.Data;
			imgData.Width = i->second.Width;
			imgData.Height = i->second.Height;

			Image::LastBound = InsertImage(i->first, &imgData);

			SOIL_free_image_data(i->second.Data);
		}

		PendingUploads.clear();
		LoadMutex.unlock();
	}

	if (Textures.size())
	{
		for (std::map<GString, Image*>::iterator i = Textures.begin(); i != Textures.end(); i++)
		{
			if (i->second->IsValid) /* all of them are valid */
				break;

			if (Load(i->first) == NULL) // If we failed loading it no need to try every. single. time.
			{
				Textures.erase(i--);
				if (i == Textures.end()) break;
			}
		}
	}
}
