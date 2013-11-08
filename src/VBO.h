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
	bool IsValid;
	Type mType;
	float VboData[12];
public:
	VBO(Type T);
	~VBO();
	void Invalidate();
	void Validate();
	void Bind();

	/* expects array of size 12 */
	void AssignData(float *Data);
};

#endif