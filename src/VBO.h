#ifndef VBO_H_
#define VBO_H_

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

private:
	/* GLuint */ uint32 InternalVBO;
	static uint32 LastBound;
	bool IsValid;
	Type mType;
	float *VboData;
	uint32 ElementCount;
public:
	VBO(Type T, uint32 Elements);
	~VBO();
	void Invalidate();
	void Validate();
	void Bind();
	uint32 GetElementCount();

	/* expects array of size 12 */
	void AssignData(float *Data);
};

#endif