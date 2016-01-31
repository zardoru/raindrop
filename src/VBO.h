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
	/* GLuint */ uint32 InternalVBO;
	static uint32 LastBound;
	static uint32 LastBoundIndex;
	bool IsValid;
	Type mType;
	void *VboData;
	IdxKind mKind;
	uint32 ElementCount;
	uint32 ElementSize;
public:
	VBO(Type T, uint32 Elements, uint32 Size = sizeof(float), IdxKind Kind = ArrayBuffer);
	~VBO();
	void Invalidate();
	void Validate();
	void Bind();
	uint32 GetElementCount();

	/* Size must be valid with parameters given to VBO. */
	void AssignData(void *Data);
};