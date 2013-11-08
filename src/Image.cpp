#include <string>
#include "Image.h"

Image* Image::LastBound = NULL;

Image::Image(unsigned int texture, int w, int h) :
  texture(texture),
  h(h),
  w(w)
{
	IsValid = false;
}

  Image::~Image()
  {
  }