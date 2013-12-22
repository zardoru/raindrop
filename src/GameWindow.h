/* Handle seriously boring stuff. */

#ifndef GraphMan_H_
#define GraphMan_H_

class VBO;
struct GLFWwindow;

class GameWindow
{
	friend void ResizeFunc(GLFWwindow*, int32,int32);
	friend void InputFunc(GLFWwindow*,int32,int32,int32,int32);
	glm::vec2 size;
	glm::vec2 matrixSize, Viewport;
	glm::mat4 projection;

	bool FullscreenSwitchbackPending, IsFullscreen;
	uint32 defaultVertexShader, defaultFragShader, defaultShaderProgram, defaultVao;
	GLFWwindow *wnd;
	float SizeRatio;

	void SetupWindow();
	void SetupShaders();
	void AssignSize();
	
	std::vector<VBO*> VBOList;

public:
	GameWindow();
	bool isGuiInputEnabled;
	void AutoSetupWindow();
	void ClearWindow(); // basically wrapping up glClear
	void Cleanup();

	// assigns this matrix on the stack
	void SetMatrix(uint32 MatrixMode, glm::mat4 matrix);
	void SetVisibleCursor(bool Visible);
	void AddVBO(VBO* V);
	void RemoveVBO(VBO *V);

	// returns the mouse position relative to the matrix and window size
	glm::vec2 GetRelativeMPos();

	// returns the size of the window
	glm::vec2 GetWindowSize() const;

	// returns the size of the orthogonal matrix
	glm::vec2 GetMatrixSize() const;

	int32 GetDefaultFragShader() const;
	int32 GetDefaultVertexShader() const;
	int32 GetShaderProgram() const;

	void SetUniform(String Uniform, int i) const;
	void SetUniform(String Uniform, float A, float B, float C, float D)  const;
	void SetUniform(String Uniform, float *Matrix4x4)  const;
	void SetUniform(String Uniform, glm::vec3 Vec)  const;
	void SetUniform(String Uniform, float F)  const;

	/* using OpenGL -1 to 1 range*/
	void SetLightPosition(glm::vec3 Position)  const;
	void SetLightMultiplier(float Multiplier)  const;
	int EnableAttribArray(String Attrib)  const;
	int DisableAttribArray(String Attrib)  const;

	bool ShouldCloseWindow();
	void SwapBuffers();
};

extern GameWindow WindowFrame;

#endif
