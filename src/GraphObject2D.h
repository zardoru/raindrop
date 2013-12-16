#ifndef GraphObject2D_H
#define GraphObject2D_H

class VBO;
class Image;

class GraphObject2D
{
	static VBO *mBuffer;
	static bool IsInitialized;
	glm::mat4x4 Matrix;

private: // Transformations

	void Cleanup();

	bool DirtyMatrix, DirtyTexture;
	uint32 mWidth, mHeight;
	glm::vec2 mPosition;
	// uint32 z_order;
	
	// These crop variables define where to crop the image.
	

	/*
	* crop_x1 and crop_y1 define the top-left corner of the crop rectangle.
	* crop_x2 and crop_y2 define the bottom-right corner of the crop rectangle.
	* 
	* These coordinates are in pixels.
	*/

	float mCrop_x1, mCrop_y1;
	float mCrop_x2, mCrop_y2;

	glm::vec2 mScale;
	float mRotation;

	Image* mImage;

protected:
	VBO *UvBuffer;
	bool DoTextureCleanup;
	void UpdateTexture();
public:
	
	GraphObject2D(bool ShouldInitTexture = true);
	~GraphObject2D();
	
	// color and other transformations
	float Alpha;
	float Red, Blue, Green;

	bool Centered; // 0 for topleft, 1 for center

	bool ColorInvert;
	bool AffectedByLightning;
	
	void SetImage(Image* image);
	Image* GetImage();

	virtual void InitVBO();
	virtual void Initialize(bool ShouldInitTexture);

	// Scale
	void SetScale(glm::vec2 Scale);
	void SetScale(float Scale);
	void SetScaleX(float ScaleX);
	void SetScaleY(float ScaleY);
	glm::vec2 GetScale() const;

	// Position
	void SetPosition(glm::vec2 Pos);
	void SetPosition(float pX, float pY);
	void AddPosition(float pX, float pY);
	void AddPosition(glm::vec2 pos);
	void SetPositionX(float pX);
	void SetPositionY(float pY);
	glm::vec2 GetPosition() const;

	// Size
	void SetSize(glm::vec2 Size);
	void SetSize(float Size);
	void SetSize(float W, float H);
	void SetWidth(uint32 W);
	void SetHeight(uint32 H);
	float GetWidth() const;
	float GetHeight() const;
	glm::vec2 GetSize() const;

	// Cropping
	void SetCrop(glm::vec2 Crop1, glm::vec2 Crop2);
	void SetCrop1(glm::vec2 Crop1);
	void SetCrop2(glm::vec2 Crop2);
	void SetCropToWholeImage();
	void SetCropByPixels(int32 x1, int32 x2, int32 y1, int32 y2);

	// Rotation
	void SetRotation(float Rot);
	float GetRotation() const;
	void AddRotation(float Rot);

	void Render(); // found in backend.cpp
	virtual void Invalidate();

	static void BindTopLeftVBO();
};

#endif