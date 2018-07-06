#include "pch.h"

#include "Texture.h"
#include "Rendering.h"
#include "ImageLoader.h"

Texture* Texture::LastBound = NULL;

Texture::Texture(unsigned int texture, int w, int h) :
    texture(texture),
    h(h),
    w(w)
{
    IsValid = false;
    TextureAssigned = true;
}

Texture::Texture()
{
    TextureAssigned = false;
    IsValid = false;
    texture = -1;
    h = -1;
    w = -1;
}

void Texture::ForceRebind()
{
    LastBound = NULL;
}


void Texture::CreateTexture()
{
	if (texture == -1 || !IsValid)
	{
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);

		LastBound = this;
		IsValid = true;
	} else if (texture != -1 && IsValid) {
		glBindTexture(GL_TEXTURE_2D, texture);

		LastBound = this;
	}
}

void Texture::Unbind()
{
	glBindTexture(GL_TEXTURE_2D, 0);
	LastBound = NULL;
}

bool Texture::IsBound()
{
	return LastBound == this;
}

void Texture::Bind()
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

void Texture::Destroy() // Called at destruction time
{
	if (IsValid && texture != -1)
	{
		/*if (ImageLoaderMessages)
			Log::LogPrintf("Texture: Destroying image %s (Removing texture...)\n", fname.string().c_str());*/
		glDeleteTextures(1, &texture);
		IsValid = false;
		texture = -1;
	}
}

void Texture::SetTextureData2D(ImageData &ImgInfo, bool Reassign)
{
	if (Reassign) Destroy();

	CreateTexture(); // Make sure our texture exists.

	if (ImgInfo.Data.size() == 0 && !Reassign)
	{
		return;
	}

	auto img = ImgInfo.TempData ? ImgInfo.TempData : ImgInfo.Data.data();

	if (!TextureAssigned || Reassign) // We haven't set any data to this texture yet, or we want to regenerate storage
	{
		TextureAssigned = true;
		auto Dir = ImgInfo.Filename.filename().string();

		glPixelStorei(GL_UNPACK_ALIGNMENT, ImgInfo.Alignment);
		Renderer::SetTextureParameters(Dir);

		//glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, ImgInfo.Width, ImgInfo.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, ImgInfo.Data.data());
		glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, ImgInfo.Width, ImgInfo.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, img);
	}
	else // We did, so let's update instead.
	{
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, ImgInfo.Width, ImgInfo.Height, GL_RGBA, GL_UNSIGNED_BYTE, img);
	}

	w = ImgInfo.Width;
	h = ImgInfo.Height;
	fname = ImgInfo.Filename;
}

void Texture::LoadFile(std::filesystem::path Filename, bool Regenerate)
{
	CreateTexture();

	/*if (ImageLoaderMessages)
		Log::LogPrintf("Texture: Assigning \"%s\"\n", Filename.string().c_str());*/
	auto Ret = ImageLoader::GetDataForImage(Filename);
	SetTextureData2D(Ret, Regenerate);
	fname = Filename;
}


Texture::~Texture()
{
    Destroy();
}