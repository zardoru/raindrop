/* Handle seriously boring stuff. */

#ifndef GraphMan_H_
#define GraphMan_H_

#include <list>

class VBO;
struct GLFWwindow;

class GameWindow
{
	friend void ResizeFunc(GLFWwindow*, int32,int32);
	friend void InputFunc(GLFWwindow*,int32,int32,int32,int32);
	Vec2 size;
	Vec2 matrixSize, Viewport;
	Mat4 projection;

	bool FullscreenSwitchbackPending, IsFullscreen;
	uint32 defaultVertexShader, defaultFragShader, defaultShaderProgram, defaultVao;
	GLFWwindow *wnd;
	float SizeRatio;

	void SetupWindow();
	void SetupShaders();
	void AssignSize();
	
	std::vector<VBO*> VBOList;
	std::map<String, uint32> UniformLocs;
	std::map<String, uint32> AttribLocs;

public:
	GameWindow();
	bool isGuiInputEnabled;
	void AutoSetupWindow();
	void ClearWindow(); // basically wrapping up glClear
	void Cleanup();

	// assigns this matrix on the stack
	void SetMatrix(uint32 MatrixMode, Mat4 matrix);
	void SetVisibleCursor(bool Visible);
	void AddVBO(VBO* V);
	void RemoveVBO(VBO *V);

	// returns the mouse position relative to the matrix and window size
	Vec2 GetRelativeMPos();

	// returns the size of the window
	Vec2 GetWindowSize() const;

	// returns the size of the orthogonal matrix
	Vec2 GetMatrixSize() const;

	int32 GetDefaultFragShader() const;
	int32 GetDefaultVertexShader() const;
	int32 GetShaderProgram() const;

	void SetUniform(String Uniform, int i) ;
	void SetUniform(String Uniform, float A, float B, float C, float D)  ;
	void SetUniform(String Uniform, float *Matrix4x4)  ;
	void SetUniform(String Uniform, glm::vec3 Vec)  ;
	void SetUniform(String Uniform, float F)  ;

	/* using OpenGL -1 to 1 range*/
	void SetLightPosition(glm::vec3 Position)  ;
	void SetLightMultiplier(float Multiplier)  ;
	int EnableAttribArray(String Attrib)  ;
	int DisableAttribArray(String Attrib)  ;

	bool ShouldCloseWindow();
	void SwapBuffers();
};

extern GameWindow WindowFrame;

#endif
