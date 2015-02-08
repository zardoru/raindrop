/* Handle seriously boring stuff. */

#ifndef GraphMan_H_
#define GraphMan_H_

#include <list>
#include <map>
#include <vector>

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
	friend void ResizeFunc(GLFWwindow*, int32,int32);
	friend void InputFunc(GLFWwindow*,int32,int32,int32,int32);
	friend void MouseInputFunc (GLFWwindow*, int32 key, int32 code, int32 modk);
	friend void ScrollFunc( GLFWwindow*, double xOff, double yOff );
	friend void MouseMoveFunc (GLFWwindow*,double newx, double newy);

	Vec2 size;
	Vec2 matrixSize, Viewport;
	Mat4 projection;
	Mat4 projectionInverse;

	uint32 defaultVertexShader, defaultFragShader, defaultShaderProgram, defaultVao;
	GLFWwindow *wnd;
	float SizeRatio;

	uint32 uniforms[NUM_SHADERVARS];

	bool SetupWindow();
	bool SetupShaders();
	void AssignSize();
	
	std::vector<VBO*> VBOList;
	std::vector<TruetypeFont*> TTFList;
	std::map<GString, uint32> UniformLocs;
	std::map<GString, uint32> AttribLocs;
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
