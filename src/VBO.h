#pragma once

/* Fixed 2D VBO of 6 points. To be expanded upon later. */
class VBO
{
public:

	enum Type
	{
		Static,
		Dynamic,
		Stream
	};

	enum IdxKind
	{
		ArrayBuffer,
		IndexBuffer
	};

private:
	/* GLuint */
    uint32_t InternalVBO;
	static uint32_t LastBound;
	static uint32_t LastBoundIndex;
	bool IsValid;
	Type mType;
	void *VboData;
	IdxKind mKind;
	uint32_t ElementCount;
	uint32_t ElementSize;
public:
	VBO(Type T, uint32_t Elements, uint32_t Size = sizeof(float), IdxKind Kind = ArrayBuffer);
	~VBO();
	void Invalidate();
	void Validate();
	void Bind();
	uint32_t GetElementCount();

	/* Size must be valid with parameters given to VBO. */
	void AssignData(void *Data);
};