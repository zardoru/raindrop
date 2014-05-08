/* Handle seriously boring stuff. */

#ifndef GraphMan_H_
#define GraphMan_H_

#include <list>

class VBO;
struct GLFWwindow;

enum RendererLocats
{
	A_POSITION,
	A_UV,
	U_MVP,
	U_TRANM,
	U_SIM,
	U_TRANSL,
	U_CENTERED,
	U_SMULT,
	U_COLOR,
	U_LMUL,
	U_LPOS,
	U_INVERT,
	U_LIGHT,
	U_HIDDEN,
	NUM_SHADERVARS
} ;

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

	uint32 uniforms[NUM_SHADERVARS];

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

	void SetUniform(uint32 Uniform, int i) ;
	void SetUniform(uint32 Uniform, float A, float B, float C, float D)  ;
	void SetUniform(uint32 Uniform, float *Matrix4x4)  ;
	void SetUniform(uint32 Uniform, glm::vec3 Vec)  ;
	void SetUniform(uint32 Uniform, float F)  ;

	/* using OpenGL -1 to 1 range*/
	void SetLightPosition(glm::vec3 Position)  ;
	void SetLightMultiplier(float Multiplier)  ;
	int EnableAttribArray(uint32 Attrib)  ;
	int DisableAttribArray(uint32 Attrib)  ;

	bool ShouldCloseWindow();
	void SwapBuffers();
};

extern GameWindow WindowFrame;

#endif
