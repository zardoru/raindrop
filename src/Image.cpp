#include "pch.h"

#include "Image.h"

Texture* Texture::LastBound = NULL;

Texture::Texture(unsigned int texture, int w, int h) :
    texture(texture),
    h(h),
    w(w)
{
    IsValid = false;
    TextureAssigned = true;
}

Texture::Texture()
{
    TextureAssigned = false;
    IsValid = false;
    texture = -1;
    h = -1;
    w = -1;
}

void Texture::ForceRebind()
{
    LastBound = NULL;
}

Texture::~Texture()
{
    Destroy();
}