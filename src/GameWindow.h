/* Handle seriously boring stuff. */

#ifndef GraphMan_H_
#define GraphMan_H_

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

public:
	GameWindow();
	bool isGuiInputEnabled;
	void AutoSetupWindow();
	void ClearWindow(); // basically wrapping up glClear
	void Cleanup();

	// assigns this matrix on the stack
	void SetMatrix(uint32 MatrixMode, glm::mat4 matrix);
	void SetVisibleCursor(bool Visible);

	// returns the mouse position relative to the matrix and window size
	glm::vec2 GetRelativeMPos();

	// returns the size of the window
	glm::vec2 GetWindowSize() const;

	// returns the size of the orthogonal matrix
	glm::vec2 GetMatrixSize() const;

	int32 GetDefaultFragShader() const;
	int32 GetDefaultVertexShader() const;
	int32 GetShaderProgram() const;

	void SetUniform(String Uniform, int i);
	void SetUniform(String Uniform, float A, float B, float C, float D);
	void SetUniform(String Uniform, float *Matrix4x4);
	int EnableAttribArray(String Attrib);
	int DisableAttribArray(String Attrib);

	bool ShouldCloseWindow();
	void SwapBuffers();
};

extern GameWindow WindowFrame;

#endif
