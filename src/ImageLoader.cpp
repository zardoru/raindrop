#include "pch.h"

#include "Logging.h"
#include "Configuration.h"

#include "Image.h"
#include "ImageLoader.h"
#include "Rendering.h"

std::mutex LoadMutex;
std::map<std::filesystem::path, Image*> ImageLoader::Textures;
std::map<std::filesystem::path, ImageLoader::UploadData> ImageLoader::PendingUploads;

CfgVar ImageLoaderMessages("ImageLoader", "Debug");

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
		if (ImageLoaderMessages)
			Log::LogPrintf("Image: Destroying image %s (Removing texture...)\n", fname.string().c_str());
        glDeleteTextures(1, &texture);
        IsValid = false;
        texture = -1;
    }
}

void Image::SetTextureData(ImageData &ImgInfo, bool Reassign)
{
    if (Reassign) Destroy();

    CreateTexture(); // Make sure our texture exists.
    Bind();

    if (ImgInfo.Data.size() == 0 && !Reassign)
    {
        return;
    }

    if (!TextureAssigned || Reassign) // We haven't set any data to this texture yet, or we want to regenerate storage
    {
        TextureAssigned = true;
        auto Dir = ImgInfo.Filename.filename().string();

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		Renderer::SetTextureParameters(Dir);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ImgInfo.Width, ImgInfo.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, ImgInfo.Data.data());
    }
    else // We did, so let's update instead.
    {
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, ImgInfo.Width, ImgInfo.Height, GL_RGBA, GL_UNSIGNED_BYTE, ImgInfo.Data.data());
    }

    w = ImgInfo.Width;
    h = ImgInfo.Height;
    fname = ImgInfo.Filename;
}

void Image::Assign(std::filesystem::path Filename, bool Regenerate)
{
    CreateTexture();

	if (ImageLoaderMessages)
		Log::LogPrintf("Image: Assigning \"%s\"\n", Filename.string().c_str());
    auto Ret = ImageLoader::GetDataForImage(Filename);
    SetTextureData(Ret, Regenerate);
    fname = Filename;
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
    for (auto i = Textures.begin(); i != Textures.end(); ++i) {
		if (ImageLoaderMessages) {
			Log::LogPrintf("ImageLoader: Invalidate texture %s\n", i->first.string().c_str());
		}
        i->second->IsValid = false;
    }
}

void ImageLoader::ReloadAll()
{
	UnloadAll();
	for (auto &img : Textures) {
		img.second->Assign(img.first, true);
	}
}

void ImageLoader::UnloadAll()
{
    for (auto i = Textures.begin(); i != Textures.end(); i++) {
		Log::LogPrintf("ImageLoader: Deleting texture %s\n", i->first.string().c_str());
		i->second->Destroy();
    }
}

void ImageLoader::DeleteImage(Image* &ToDelete)
{
    if (ToDelete) {
		auto tex = Textures.find(ToDelete->fname);
		if (tex != Textures.end()) {
			Textures.erase(tex);
			delete ToDelete;
			ToDelete = nullptr;
		}
		else {
			Log::LogPrintf("ImageLoader: Attempt to delete texture not registered in loader (%s)\n", ToDelete->fname.string().c_str());
		}
	}
	else {
		if (ImageLoaderMessages)
			Log::LogPrintf("ImageLoader: Attempt to destroy NULL image\n");
	}
}

Image* ImageLoader::InsertImage(std::filesystem::path Name, ImageData &imgData)
{
    Image* I;

    if (imgData.Data.size() == 0) return nullptr;

    if (Textures.find(Name) == Textures.end())
        I = (Textures[Name] = new Image());
    else
        I = Textures[Name];

    I->SetTextureData(imgData);
    I->fname = Name;

    return I;
}

template<typename Stream>
auto open_image(Stream&& in)
{
    using namespace boost::gil;

    rgba8_image_t img;
    do
    {
        try
        {
            try
            {
                read_and_convert_image(in, img, png_tag());
                break;
            }
            catch (std::ios_base::failure) {}

            in.clear();
            in.seekg(0);
            try
            {
                read_and_convert_image(in, img, jpeg_tag());
                break;
            }
            catch (std::ios_base::failure) {}

            in.clear();
            in.seekg(0);
            try
            {
                read_and_convert_image(in, img, targa_tag());
                break;
            }
            catch (std::ios_base::failure) {}

            in.clear();
            in.seekg(0);
            read_and_convert_image(in, img, bmp_tag());
        }
        catch (...)
        {
            Log::Printf("Could not load image");
        }
    } while (false);

    auto v = view(img);
    using pixel = decltype(v)::value_type;
    auto data = std::vector<uint32_t>();
	data.resize(v.width() * v.height());

	static_assert(sizeof(pixel) == sizeof(uint32_t), "Pixels are required to be RGBA 32 bits");
    copy_pixels(v, interleaved_view(v.width(), v.height(), (pixel*)data.data(),
        v.width() * sizeof(pixel)));


    ImageData out;
    out.Data.assign(data.begin(), data.end());
    out.Width = v.width();
    out.Height = v.height();

    return out;
}

const char* exts[] = {".png", ".bmp", ".tga", ".jpg", ".jpeg"};
ImageData ImageLoader::GetDataForImage(std::filesystem::path filename)
{
	if (!std::filesystem::exists(filename)) {
		auto orig_ext = filename.extension().string();
		bool found = false;

		for (auto e : exts) {
			if (e != orig_ext) {
				if (std::filesystem::exists(filename.replace_extension(e))) {
					
					if (ImageLoaderMessages)
						Log::LogPrintf("ImageLoader: Replaced extension for %s to %s for loading.\n", filename.string().c_str(), e);

					filename = filename.replace_extension(e);
					found = true;
					break;
				}
			}
		}

		if (ImageLoaderMessages && !found) {
			Log::Printf("ImageLoader: Couldn't access \"%s\"", filename.string().c_str());
			Log::Printf("\n");
		}

		if (!found)
			return {};
	}

    auto file = std::ifstream{ filename.string(), std::ios::binary };
    if (!file.is_open())
    {
		if (ImageLoaderMessages) {
			Log::Printf("ImageLoader: Unable to open \"%s\" (but file was found).\n", filename.string().c_str());
		}

        return{};
    }

    auto out = open_image(file);
    out.Filename = filename;

	if (ImageLoaderMessages)
		Log::LogPrintf("ImageLoader: \"%s\" loaded.\n", filename.string().c_str());

    return out;
}

ImageData ImageLoader::GetDataForImageFromMemory(const unsigned char* const buffer, size_t len)
{
    auto file = std::stringstream{ std::stringstream::in |
        std::stringstream::out | std::stringstream::binary };
    file.write((const char*)buffer, len);

    auto out = open_image(file);

    return out;
}

Image* ImageLoader::Load(std::filesystem::path filename)
{
	if (std::filesystem::is_directory(filename)) return NULL;
    if (Textures.find(filename) != Textures.end() && Textures[filename]->IsValid)
    {
        return Textures[filename];
    }
    else
    {
        ImageData ImgData = GetDataForImage(filename);
        Image* Ret = InsertImage(filename, ImgData);

        Image::LastBound = Ret;

        return Ret;
    }
    return 0;
}

void ImageLoader::AddToPending(std::filesystem::path Filename)
{
    UploadData New;
    if (Textures.find(Filename) == Textures.end())
    {
        auto d = GetDataForImage(Filename);
        New.Data = d.Data;
        New.Width = d.Width;
        New.Height = d.Height;
        LoadMutex.lock();
        PendingUploads.insert(std::pair<std::filesystem::path, UploadData>(Filename, New));
        LoadMutex.unlock();
    }
}

/* For multi-threaded loading. */
void ImageLoader::LoadFromManifest(char** Manifest, int Count, std::string Prefix)
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
        for (auto i = PendingUploads.begin(); i != PendingUploads.end(); i++)
        {
            ImageData imgData;
            imgData.Data = i->second.Data;
            imgData.Width = i->second.Width;
            imgData.Height = i->second.Height;

            Image::LastBound = InsertImage(i->first, imgData);
        }

        PendingUploads.clear();
        LoadMutex.unlock();
    }

    if (Textures.size())
    {
        for (auto i = Textures.begin(); i != Textures.end();)
        {
            if (i->second->IsValid) /* all of them are valid */
                break;

            if (Load(i->first) == nullptr) // If we failed loading it no need to try every. single. time.
            {
                i = Textures.erase(i);
                continue;
            }

            ++i;
        }
    }
}

void ImageLoader::RegisterTexture(Image* tex)
{
	if (tex->fname.string().length()) {
		if (Textures.find(tex->fname) != Textures.end()) {
			if (ImageLoaderMessages)
				Log::LogPrintf("ImageLoader: Attempt to manually replace texture \"%s\" from storage\n", tex->fname.string().c_str());
		}
		else
			Textures[tex->fname] = tex;
	}
}