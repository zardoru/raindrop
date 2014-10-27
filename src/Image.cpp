#include "Global.h"
#include "Image.h"

Image* Image::LastBound = NULL;

Image::Image(unsigned int texture, int w, int h) :
  texture(texture),
  h(h),
  w(w)
{
	IsValid = false;
}

  void Image::ForceRebind()
  {
	  LastBound = NULL;
  }

  Image::~Image()
  {
  }