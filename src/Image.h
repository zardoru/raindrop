#ifndef Image_H
#define Image_H

class Image
{
	friend class ImageLoader;
	static Image* LastBound;
public:
	Image(unsigned int texture, int w, int h);
	~Image();
	void Bind();

	std::string fname;
	int w, h;
	unsigned int texture;
	bool IsValid;
};

#endif
