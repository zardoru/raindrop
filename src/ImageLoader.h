#ifndef ImageLoader_H
#define ImageLoader_H

#include <map>
#include "Image.h"

class ImageLoader
{
private:

	static std::map<std::string, Image*> Textures;
	
public:
	
	ImageLoader();
	~ImageLoader();
	
	static Image* Load(std::string filename);
	static Image* LoadSkin(std::string filename);
};

#endif