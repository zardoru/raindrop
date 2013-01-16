#ifndef Image_H
#define Image_H

class Image
{
public:
	Image(unsigned int texture, int w, int h);
	~Image();
	
	int w, h;
	unsigned int texture;
};

#endif
