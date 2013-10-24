#include <string>
#include "Image.h"

Image::Image(unsigned int texture, int w, int h) :
  texture(texture),
  w(w),
  h(h)
{
	IsValid = false;
}

  Image::~Image()
  {
  }