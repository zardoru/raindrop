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

	// returns the mouse position relative to the matrix and window size
	glm::vec2 GetRelativeMPos();

	// returns the size of the window
	glm::vec2 GetWindowSize();

	// returns the size of the orthogonal matrix
	glm::vec2 GetMatrixSize();

	int32 GetDefaultFragShader();
	int32 GetDefaultVertexShader();
	int32 GetShaderProgram();

	bool ShouldCloseWindow();
	void SwapBuffers();
};

extern GameWindow WindowFrame;

#endif
