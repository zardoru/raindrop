#include "pch.h"


#include "Image.h"

Image* Image::LastBound = NULL;

Image::Image(unsigned int texture, int w, int h) :
  texture(texture),
  h(h),
  w(w)
{
	IsValid = false;
	TextureAssigned = true;
}

Image::Image()
{
	TextureAssigned = false;
	IsValid = false;
	texture = -1;
	h = -1;
	w = -1;
}

  void Image::ForceRebind()
  {
	  LastBound = NULL;
  }

  Image::~Image()
  {
	  Destroy();
  }