#ifndef GraphObject2D_H
#define GraphObject2D_H

class VBO;
class Image;

enum rBlendMode
{
	MODE_ADD,
	MODE_ALPHA,
};

class GraphObject2D
{
	static VBO *mBuffer;
	static bool IsInitialized;
	glm::mat4x4 Matrix;

private: // Transformations

	void Cleanup();

	bool DirtyMatrix, DirtyTexture;
	uint32 mWidth, mHeight;
	Vec2 mPosition;
	uint32 z_order;
	
	rBlendMode BlendingMode;

	// These crop variables define where to crop the image.
	

	/*
	* crop_x1 and crop_y1 define the top-left corner of the crop rectangle.
	* crop_x2 and crop_y2 define the bottom-right corner of the crop rectangle.
	* 
	* These coordinates are in pixels.
	*/

	float mCrop_x1, mCrop_y1;
	float mCrop_x2, mCrop_y2;

	Vec2 mScale;
	float mRotation;

	Image* mImage;

protected:
	VBO *UvBuffer;
	bool DoTextureCleanup;
	void UpdateTexture();
	void UpdateMatrix();

public:
	
	GraphObject2D(bool ShouldInitTexture = true);
	~GraphObject2D();
	
	// color and other transformations
	float Alpha;
	float Red, Blue, Green;

	bool Centered; // 0 for topleft, 1 for center

	bool ColorInvert;
	bool AffectedByLightning;
	
	void SetImage(Image* image, bool ChangeSize = true);
	Image* GetImage();

	virtual void InitVBO();
	virtual void Initialize(bool ShouldInitTexture);

	void SetBlendMode(rBlendMode Mode);

	// Scale
	void SetScale(Vec2 Scale);
	void SetScale(float Scale);
	void SetScaleX(float ScaleX);
	void SetScaleY(float ScaleY);
	Vec2 GetScale() const;

	// Position
	void SetPosition(Vec2 Pos);
	void SetPosition(float pX, float pY);
	void AddPosition(float pX, float pY);
	void AddPosition(Vec2 pos);
	void SetPositionX(float pX);
	void SetPositionY(float pY);
	Vec2 GetPosition() const;
	void SetZ(uint32 Z);
	uint32 GetZ() const;

	// Size
	void SetSize(Vec2 Size);
	void SetSize(float Size);
	void SetSize(float W, float H);
	void SetWidth(uint32 W);
	void SetHeight(uint32 H);
	float GetWidth() const;
	float GetHeight() const;
	Vec2 GetSize() const;

	// Cropping
	void SetCrop(Vec2 Crop1, Vec2 Crop2);
	void SetCrop1(Vec2 Crop1);
	void SetCrop2(Vec2 Crop2);
	void SetCropToWholeImage();
	void SetCropByPixels(int32 x1, int32 x2, int32 y1, int32 y2);

	// Rotation
	void SetRotation(float Rot);
	float GetRotation() const;
	void AddRotation(float Rot);

	const glm::mat4 &GetMatrix();
	bool ShouldUpdateMatrix() const;

	void Render(); // found in backend.cpp
	virtual void Invalidate();

	static void BindTopLeftVBO();
	void BindTextureVBO();
};

#endif