#ifndef Sprite_H
#define Sprite_H

#include "Rendering.h"
#include "Transformation.h"
#include "Drawable.h"

class VBO;
class Image;

class Drawable2D : public Transformation, public Drawable
{
public:
	virtual ~Drawable2D() {};
	virtual void Render();
	// Stub
};

class Sprite : public Drawable2D
{
private: // Transformations
	void Cleanup();

	bool DirtyTexture;
	
	RBlendMode BlendingMode;

	// These crop variables define where to crop the image.

	/*
	* crop_x1 and crop_y1 define the top-left corner of the crop rectangle.
	* crop_x2 and crop_y2 define the bottom-right corner of the crop rectangle.
	* 
	* These coordinates are in fractions.
	*/

	float mCrop_x1, mCrop_y1;
	float mCrop_x2, mCrop_y2;

	Image* mImage;

	void Construct(bool doInitTexture);
protected:
	VBO *UvBuffer;
	bool DoTextureCleanup;
	void UpdateTexture();
public:
	
	Sprite(bool ShouldInitTexture);
	Sprite();
	~Sprite();
	
	// color and other transformations
	float Alpha;
	float Red, Blue, Green;

	// Only valid if lighten is enabled.
	float LightenFactor; 

	bool Centered; // 0 for topleft, 1 for center
	bool Lighten;

	bool ColorInvert;
	bool AffectedByLightning;
	bool BlackToTransparent; // If enabled, transforms black pixels into transparent pixels.

	void SetImage(Image* image, bool ChangeSize = true);
	Image* GetImage();
	GString GetImageFilename() const;

	virtual void Initialize(bool ShouldInitTexture);

	void SetBlendMode(int Mode);
	int GetBlendMode() const;


	// Cropping
	void SetCrop(Vec2 Crop1, Vec2 Crop2);
	void SetCrop1(Vec2 Crop1);
	void SetCrop2(Vec2 Crop2);
	void SetCropToWholeImage();
	void SetCropByPixels(int32 x1, int32 x2, int32 y1, int32 y2);

	virtual void Render(); // found in backend.cpp
	virtual void Invalidate();

	void BindTextureVBO();
};

#endif