#include <filesystem>
#include <GL/glew.h>
#include <rmath.h>
#include <map>

#include "Transformation.h"
#include "Texture2D.h"
#include "Rendering.h"
#include "TextureCollection.h"

Texture2D* Texture2D::last_bound_ = nullptr;

Texture2D::Texture2D(const unsigned int texture, const int w, const int h) :
    w(w),
    h(h),
    texture(texture)
{
    is_valid_ = false;
    texture_was_assigned_ = true;
}

Texture2D::Texture2D()
{
    texture_was_assigned_ = false;
    is_valid_ = false;
    texture = -1;
    h = -1;
    w = -1;
}

void Texture2D::force_rebind()
{
    last_bound_ = nullptr;
}


void Texture2D::ensure_current_gpu_texture_2d()
{
	if (texture == -1 || !is_valid_)
	{
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);

		is_valid_ = true;
	} else if (texture != -1 && is_valid_) {
		glBindTexture(GL_TEXTURE_2D, texture);
	}

    last_bound_ = this;
}

void Texture2D::unbind()
{
	glBindTexture(GL_TEXTURE_2D, 0);
	last_bound_ = nullptr;
}

bool Texture2D::is_bound() const {
	return last_bound_ == this;
}

void Texture2D::bind()
{
	if (is_valid_ && texture != -1)
	{
		if (last_bound_ != this)
		{
			glBindTexture(GL_TEXTURE_2D, texture);
			last_bound_ = this;
		}
	}
}

void Texture2D::destroy() // Called at destruction time
{
	if (is_valid_ && texture != -1)
	{
		glDeleteTextures(1, &texture);
		is_valid_ = false;
		texture = -1;
	}
}

void Texture2D::set_texture_data_2d(const ImageData2d &img_info, const bool regenerate)
{
	if (regenerate) destroy();

	ensure_current_gpu_texture_2d(); // Make sure our texture exists.

	if (img_info.Data.empty() && !regenerate)
	{
		return;
	}

	const auto img = img_info.TempData ? img_info.TempData : img_info.Data.data();

	if (!texture_was_assigned_ || regenerate) // We haven't set any data to this texture yet, or we want to regenerate storage
	{
		texture_was_assigned_ = true;
		auto Dir = img_info.Filename.filename().string();

		glPixelStorei(GL_UNPACK_ALIGNMENT, img_info.Alignment);

		renderer::set_texture_parameters(Dir);
        assert (glGetError() == 0);

		//glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, ImgInfo.Width, ImgInfo.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, ImgInfo.Data.data());
		glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, img_info.Width, img_info.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, img);
        assert (glGetError() == 0);
	}
	else // We did, so let's update instead.
	{
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, img_info.Width, img_info.Height, GL_RGBA, GL_UNSIGNED_BYTE, img);
	}

	w = img_info.Width;
	h = img_info.Height;
	fname = img_info.Filename;
}

void Texture2D::load_file(const std::filesystem::path &filename, const bool regenerate)
{
	ensure_current_gpu_texture_2d();

	const auto Ret = TextureCollection::get_data_for_image(filename);
	set_texture_data_2d(Ret, regenerate);
	fname = filename;
}


Texture2D::~Texture2D()
{
    destroy();
}