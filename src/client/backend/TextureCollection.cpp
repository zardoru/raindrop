#include <mutex>
#include <map>
#include <filesystem>
#include <rmath.h>
#include <fstream>
#include <sstream>

#include "Logging.h"

#include <boost/gil/extension/io/bmp.hpp>
#include <boost/gil/extension/io/png.hpp>
#include <boost/gil/extension/io/jpeg.hpp>
#include <boost/gil/extension/io/targa.hpp>


#include "Texture2D.h"
#include "TextureCollection.h"
#include "Rendering.h"

#include "../structure/Configuration.h"

std::mutex load_mutex;
std::map<std::filesystem::path, Texture2D*> TextureCollection::textures_;
std::map<std::filesystem::path, TextureCollection::UploadData> TextureCollection::pending_uploads_;

CfgVar image_loader_messages("ImageLoader", "Debug");
CfgVar xor_texture("XorTexture", "Debug");

Texture2D* TextureCollection::get_texture_2d_from_file(const std::filesystem::path& name, const ImageData2d &img_data)
{
    Texture2D* I;
    if (xor_texture) return renderer::get_xor_texture();

    if (img_data.Data.empty()) return nullptr;

    if (!textures_.contains(name))
        I = (textures_[name] = new Texture2D());
    else
        I = textures_[name];

    I->set_texture_data_2d(img_data);
    I->fname = name;

    return I;
}

TextureCollection::TextureCollection()
= default;

TextureCollection::~TextureCollection()
{
    // unload ALL the images.
}

void TextureCollection::invalidate_all()
{
    for (auto & Texture : textures_) {
        if (image_loader_messages) {
            Log::LogPrintf("ImageLoader: Invalidate texture %ls\n", Texture.first.wstring().c_str());
        }
        Texture.second->is_valid_ = false;
    }
}

void TextureCollection::unload_all()
{
    for (auto & texture : textures_) {
        Log::LogPrintf("ImageLoader: Deleting texture %s\n", texture.first.string().c_str());
        texture.second->destroy();
    }
}

void TextureCollection::delete_texture_2d(Texture2D* &to_delete)
{
    if (to_delete == renderer::get_xor_texture()) return;

    if (to_delete) {
        auto tex = textures_.find(to_delete->fname);
        if (tex != textures_.end()) {
            textures_.erase(tex);
            delete to_delete;
            to_delete = nullptr;
        }
        else {
            Log::LogPrintf("ImageLoader: Attempt to delete texture not registered in loader (%ls)\n", to_delete->fname.wstring().c_str());
        }
    } else {
        if (image_loader_messages)
            Log::LogPrintf("ImageLoader: Attempt to destroy NULL image\n");
    }
}

void TextureCollection::reload_all()
{
    unload_all();
    for (auto &[path, tex] : textures_) {
        tex->load_file(path, true);
    }
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
                // Log::LogPrintf("io exception loading file %s\n", f.what());
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


    ImageData2d out;
    out.Data.assign(data.begin(), data.end());
    out.Width = v.width();
    out.Height = v.height();

    return out;
}

const char* exts[] = {".png", ".bmp", ".tga", ".jpg", ".jpeg"};
ImageData2d TextureCollection::get_data_for_image(std::filesystem::path filename)
{
	if (!std::filesystem::exists(filename)) {
		auto orig_ext = filename.extension().string();
		bool found = false;

		for (auto e : exts) {
			if (e != orig_ext) {
				if (std::filesystem::exists(filename.replace_extension(e))) {
					
					if (image_loader_messages)
						Log::LogPrintf("ImageLoader: Replaced extension for %ls to %s for loading.\n", filename.wstring().c_str(), e);

					filename = filename.replace_extension(e);
					found = true;
					break;
				}
			}
		}

		if (image_loader_messages && !found) {
			Log::Printf("ImageLoader: Couldn't access \"%ls\"", filename.wstring().c_str());
			Log::Printf("\n");
		}

		if (!found)
			return {};
	}

	// this macro warps around windows/linux stuff wrt wide strings
	std::ifstream file (filename, std::ios::binary);
    if (!file.is_open())
    {
		if (image_loader_messages) {
			Log::Printf("ImageLoader: Unable to open \"%ls\" (but file was found).\n", filename.wstring().c_str());
		}

        return{};
    }

    auto out = open_image(file);
    out.Filename = filename;

	if (image_loader_messages)
		Log::LogPrintf("ImageLoader: \"%ls\" loaded.\n", filename.wstring().c_str());

    return out;
}

ImageData2d TextureCollection::get_data_for_image_from_memory(const unsigned char* const buffer, const size_t len)
{
    auto file = std::stringstream{ std::stringstream::in |
        std::stringstream::out | std::stringstream::binary };
    file.write((const char*)buffer, len);

    auto out = open_image(file);

    return out;
}

Texture2D* TextureCollection::load(const std::filesystem::path& filename)
{
    if (xor_texture) return renderer::get_xor_texture();

	if (std::filesystem::is_directory(filename)) return nullptr;
    if (textures_.contains(filename) && textures_[filename]->is_valid_)
    {
        return textures_[filename];
    }
    else
    {
        const ImageData2d img_data = get_data_for_image(filename);
        Texture2D* ret = get_texture_2d_from_file(filename, img_data);

        Texture2D::last_bound_ = ret;

        return ret;
    }
    return nullptr;
}

void TextureCollection::add_to_pending_2d_uploads(const std::filesystem::path& filename)
{
    if (xor_texture) return;

    if (!textures_.contains(filename))
    {
        UploadData New;
        const auto d = get_data_for_image(filename);
        New.Data = d.Data;
        New.Width = d.Width;
        New.Height = d.Height;
        load_mutex.lock();
        pending_uploads_.insert(std::pair<std::filesystem::path, UploadData>(filename, New));
        load_mutex.unlock();
    }
}

/* For multi-threaded loading. */
void TextureCollection::load_from_manifest(const char** manifest, const int count, const std::string& prefix)
{
    for (int i = 0; i < count; i++)
    {
        auto final_filename = prefix + manifest[i];
        add_to_pending_2d_uploads(final_filename);
    }
}

void TextureCollection::upload_and_reload_textures()
{
    if (!pending_uploads_.empty() && load_mutex.try_lock())
    {
        for (auto &[path, upload] : pending_uploads_)
        {
            ImageData2d img_data;
            img_data.Data = upload.Data;
            img_data.Width = upload.Width;
            img_data.Height = upload.Height;

            Texture2D::last_bound_ = get_texture_2d_from_file(path, img_data);
        }

        pending_uploads_.clear();
        load_mutex.unlock();
    }

    if (!textures_.empty())
    {
        for (auto i = textures_.begin(); i != textures_.end();)
        {
            if (i->second->is_valid_) /* all of them are valid */
                break;

            if (load(i->first) == nullptr) // If we failed loading it no need to try every. single. time.
            {
                i = textures_.erase(i);
                continue;
            }

            ++i;
        }
    }
}

void TextureCollection::register_texture(Texture2D* tex)
{
	if (!tex->fname.string().empty()) {
		if (textures_.contains(tex->fname)) {
			if (image_loader_messages)
				Log::LogPrintf("ImageLoader: Attempt to manually replace texture \"%ls\" from storage\n", tex->fname.wstring().c_str());
		}
		else
			textures_[tex->fname] = tex;
	}
}