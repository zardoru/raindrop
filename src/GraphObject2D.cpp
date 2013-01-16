#include "Global.h"
#include "GraphObject2D.h"

#include "Image.h"

GraphObject2D::GraphObject2D()
{
	alpha = 1.0;
	setCropToWholeImage();
	width = height = 0;
	position = glm::vec2(0, 0);
	z_order = 0;
	red = blue = green = 1.0;
	scaleX = scaleY = 1.0;
	rotation = 0;
	origin = 0;
	IsInitialized = false;
}

GraphObject2D::~GraphObject2D()
{
}

void GraphObject2D::setImage(Image* image){
	mImage = image;
	setCropToWholeImage();
	if (image) // better safe than sorry
	{
		width = image->w;
		height = image->h;
	}
}

void GraphObject2D::setCropByPixels(int x1, int x2, int y1, int y2){
	crop_x1 = (float) x1 / (float) width;
	crop_x2 = (float) x2 / (float) width;
	crop_y1 = (float) y1 / (float) height;
	crop_y2 = (float) y2 / (float) height;
	Init();
}

void GraphObject2D::setCropToWholeImage(){
	crop_x1 = 0;
	crop_x2 = 1;
	crop_y1 = 0;
	crop_y2 = 1;
}