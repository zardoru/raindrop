#pragma once

class VBO;
class Application;
class TruetypeFont;
struct GLFWwindow;

enum RendererLocats
{
	A_POSITION,
	A_UV,
	A_COLOR,
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
	U_HIDLOW,
	U_HIDHIGH,
	U_HIDFAC,
	U_HIDSUM,
	U_REPCOLOR,
	U_BTRANSP,
	NUM_SHADERVARS
} ;

class GameWindow
{
	friend void ResizeFunc(GLFWwindow*, int32_t,int32_t);
	friend void InputFunc(GLFWwindow*,int32_t,int32_t,int32_t,int32_t);
	friend void MouseInputFunc (GLFWwindow*, int32_t key, int32_t code, int32_t modk);
	friend void ScrollFunc( GLFWwindow*, double xOff, double yOff );
	friend void MouseMoveFunc (GLFWwindow*,double newx, double newy);
	friend void CharInputFunc(GLFWwindow*, unsigned int);

	Vec2 size;
	Vec2 matrixSize, Viewport;
	Mat4 projection;
	Mat4 projectionInverse;

	uint32_t defaultVertexShader, defaultFragShader, defaultShaderProgram, defaultVao;
	GLFWwindow *wnd;
	float SizeRatio;

	uint32_t uniforms[NUM_SHADERVARS];

	bool SetupWindow();
	bool SetupShaders();
	void AssignSize();
	
	std::vector<VBO*> VBOList;
	std::vector<TruetypeFont*> TTFList;
	std::map<std::string, uint32_t> UniformLocs;
	std::map<std::string, uint32_t> AttribLocs;
	Application* Parent;

	bool FullscreenSwitchbackPending, IsFullscreen;


public:
	GameWindow();
	bool AutoSetupWindow(Application* Parent);
	void ClearWindow(); // basically wrapping up glClear
	void Cleanup();

	void SetVisibleCursor(bool Visible);
	void AddVBO(VBO* V);
	void RemoveVBO(VBO *V);

	void AddTTF(TruetypeFont* TTF);
	void RemoveTTF(TruetypeFont* TTF);

	Mat4 GetMatrixProjection();
	Mat4 GetMatrixProjectionInverse();

	float GetWindowVScale();

	// returns the mouse position relative to the matrix and window size
	Vec2 GetRelativeMPos();
	
	// returns the mouse position relative to the window.
	Vec2 GetWindowMPos();

	// returns the size of the window
	Vec2 GetWindowSize() const;

	// returns the size of the orthogonal matrix
	Vec2 GetMatrixSize() const;

	int32_t GetDefaultFragShader() const;
	int32_t GetDefaultVertexShader() const;
	int32_t GetShaderProgram() const;

	void SetUniform(uint32_t Uniform, int i) ;
	void SetUniform(uint32_t Uniform, float A, float B, float C, float D)  ;
	void SetUniform(uint32_t Uniform, float *Matrix4x4)  ;
	void SetUniform(uint32_t Uniform, glm::vec3 Vec)  ;
	void SetUniform(uint32_t Uniform, float F)  ;

	/* using OpenGL -1 to 1 range*/
	void SetLightPosition(glm::vec3 Position)  ;
	void SetLightMultiplier(float Multiplier)  ;
	int EnableAttribArray(uint32_t Attrib)  ;
	int DisableAttribArray(uint32_t Attrib)  ;

	bool ShouldCloseWindow();
	void SwapBuffers();
};

extern GameWindow WindowFrame;
