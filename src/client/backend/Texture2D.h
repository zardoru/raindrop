#pragma once

#include <vector>

struct ImageData2d
{
	std::filesystem::path Filename;
    int Width, Height, Alignment;
    std::vector<uint32_t> Data;
	
	// If you can't copy into Data, then fill this pointer instead.
	uint32_t* TempData;

    ImageData2d()
    {
		Alignment = 1;
        Width = 0; Height = 0;
		TempData = nullptr;
    }

	ImageData2d(const int w, const int h, void* data, const int align = 1) {
		ImageData2d();
		Width = w; Height = h;
		TempData = (uint32_t*)data;
		Alignment = align;

	}
};

class Texture2D
{
    friend class TextureCollection;
    static Texture2D* last_bound_;

    void destroy();

protected:
	bool texture_was_assigned_;
    void ensure_current_gpu_texture_2d();
public:
    Texture2D(unsigned int texture, int w, int h);
    Texture2D();
    virtual ~Texture2D();

	bool is_bound() const;
    void bind();
    void load_file(const std::filesystem::path &filename, bool regenerate = false);
    void set_texture_data_2d(const ImageData2d &Data, bool regenerate = false);

    // Utilitarian
    static void force_rebind();
    static void unbind(); // Or, basically unbind.

    // Data
	std::filesystem::path fname;
    int w, h;
    unsigned int texture;
    bool is_valid_;
};
