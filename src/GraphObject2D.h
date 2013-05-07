#ifndef GraphObject2D_H
#define GraphObject2D_H

class Image;

class GraphObject2D
{
	uint32 ourBuffer, ourUVBuffer;
	bool IsInitialized;
public:

	GraphObject2D();
	~GraphObject2D();
	
	uint32 width, height;
	glm::vec2 position;
	uint32 z_order;
	
	// These crop variables define where to crop the image.
	
	float crop_x1, crop_y1;
	float crop_x2, crop_y2;

	// color and other transformations
	float alpha;
	float red, blue, green;
	float scaleX, scaleY;
	float rotation;
	bool Centered; // 0 for topleft, 1 for center
	
	/*
	 * crop_x1 and crop_y1 define the top-left corner of the crop rectangle.
	 * crop_x2 and crop_y2 define the bottom-right corner of the crop rectangle.
	 * 
	 * These coordinates are in pixels.
	 */
	
	Image* mImage;
	void setImage(Image* image);

	virtual void Init(bool GenBuffers = false);
	void Render(); // found in backend.cpp

	void setCropToWholeImage();
	void setCropByPixels(int32 x1, int32 x2, int32 y1, int32 y2);
	
};

#endif