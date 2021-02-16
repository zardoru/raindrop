#include <mutex>
#include <map>
#include <filesystem>
#include <rmath.h>
#include <fstream>
#include <sstream>

#include "../Logging.h"

#include <boost/gil/extension/io/bmp.hpp>
#include <boost/gil/extension/io/png.hpp>
#include <boost/gil/extension/io/jpeg.hpp>
#include <boost/gil/extension/io/targa.hpp>


#include "Texture.h"
#include "ImageLoader.h"
#include "Transformation.h"
#include "Rendering.h"

#include "../Configuration.h"

std::mutex LoadMutex;
std::map<std::filesystem::path, Texture*> ImageLoader::Textures;
std::map<std::filesystem::path, ImageLoader::UploadData> ImageLoader::PendingUploads;

CfgVar ImageLoaderMessages("ImageLoader", "Debug");
CfgVar XorTexture("XorTexture", "Debug");

ImageLoader::ImageLoader()
= default;

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
		img.second->LoadFile(img.first, true);
	}
}

void ImageLoader::UnloadAll()
{
    for (auto & Texture : Textures) {
		Log::LogPrintf("ImageLoader: Deleting texture %s\n", Texture.first.string().c_str());
		Texture.second->Destroy();
    }
}

void ImageLoader::DeleteImage(Texture* &ToDelete)
{
    if (ToDelete == Renderer::GetXorTexture()) return;

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

Texture* ImageLoader::InsertImage(std::filesystem::path Name, ImageData &imgData)
{
    Texture* I;
    if (XorTexture) return Renderer::GetXorTexture();

    if (imgData.Data.size() == 0) return nullptr;

    if (Textures.find(Name) == Textures.end())
        I = (Textures[Name] = new Texture());
    else
        I = Textures[Name];

    I->SetTextureData2D(imgData);
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
            catch (std::ios_base::failure& f) {
                Log::LogPrintf("io exception loading file %s\n", f.what());
            }

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

	// this macro warps around windows/linux stuff wrt wide strings
	std::ifstream file (filename, std::ios::binary);
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

Texture* ImageLoader::Load(const std::filesystem::path& filename)
{
    if (XorTexture) return Renderer::GetXorTexture();

	if (std::filesystem::is_directory(filename)) return NULL;
    if (Textures.find(filename) != Textures.end() && Textures[filename]->IsValid)
    {
        return Textures[filename];
    }
    else
    {
        ImageData ImgData = GetDataForImage(filename);
        Texture* Ret = InsertImage(filename, ImgData);

        Texture::LastBound = Ret;

        return Ret;
    }
    return 0;
}

void ImageLoader::AddToPending(const std::filesystem::path& Filename)
{
    UploadData New;

    if (XorTexture) return;

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
void ImageLoader::LoadFromManifest(const char** Manifest, int Count, std::string Prefix)
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

            Texture::LastBound = InsertImage(i->first, imgData);
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

void ImageLoader::RegisterTexture(Texture* tex)
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