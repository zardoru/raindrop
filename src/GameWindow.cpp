#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "Global.h"
#include "Screen.h"
#include "Application.h"
#include "GameWindow.h"
#include "ImageLoader.h"
#include "GraphObject2D.h"
#include "GameObject.h"
#include "VBO.h"
#include <glm/gtc/matrix_transform.hpp>

#include "Configuration.h"

GameWindow WindowFrame;

const char* vertShader = "#version 120\n"
	"attribute vec3 position;\n"
	"attribute vec2 vertexUV;\n"
	"uniform mat4 projection;\n"
	"uniform mat4 mvp;\n"
	"uniform mat4 tranM;\n"
	"uniform bool useTranslate;\n"
	"uniform bool Centered;\n"
	"uniform float sMult;\n" // For 7K mode, letting the shader do the heavy lifting
	"varying vec2 Texcoord;\n"
	"varying vec3 Pos_world;\n"
	"void main() \n"
	"{\n"
	"	vec3 k_pos = position;\n"
	"	if (Centered){\n"
	"		k_pos = k_pos + vec3(-0.5, -0.5, 0);"
	"	}"
	"	if (useTranslate){\n"
	"		mat4 m = tranM, k = mvp;\n" // m = note position k = position
	"		m[3][1] *= sMult;\n"
	"		m = m * k;\n"
	"		gl_Position = projection * m * vec4(k_pos, 1);"
	"	}else{\n"
	"		gl_Position = projection * mvp * vec4(k_pos, 1);\n"
	"	}\n"
	"	Pos_world = (projection * mvp * vec4(k_pos, 1)).xyz;\n"
	"	Texcoord = vertexUV;\n"
	"}";

const char* fragShader = "#version 120\n"
	"varying vec2 Texcoord;\n"
	"varying vec3 Pos_world;\n"
	"uniform vec4 Color;\n"
	"uniform sampler2D tex;\n"
	"uniform float     lMul;\n"
	"uniform vec3      lPos;\n"
	"uniform bool inverted;\n"
	"uniform bool AffectedByLightning;\n"
	"\n"
	"void main(void)\n"
	"{\n"
	"    vec4 tCol;"
	"	 if (inverted) {\n"
	"		tCol = vec4(1.0 , 1.0, 1.0, texture2D(tex, Texcoord).a*2) - texture2D(tex, Texcoord) * Color;\n"
	"	 }else{\n"
	"		tCol = texture2D(tex, Texcoord) * Color;\n"
	"	 }\n"
	"    float dist = length ( lPos - Pos_world );\n"
	"    if (AffectedByLightning){\n"
	"       float temp = (lMul * clamp (dot( vec3(0,0,1), lPos), 0, 1)) / (dist*dist);\n"
	"		gl_FragColor = tCol * vec4(vec3(temp), 1);"
	"	 }else{\n"
	"		gl_FragColor = tCol;"
	"    }\n"
	"}\n";

std::map<int32, KeyType> BindingsManager::ScanFunction;
std::map<int32, KeyType> BindingsManager::ScanFunction7K;

void BindingsManager::Initialize()
{
	ScanFunction[GLFW_KEY_ESCAPE] = KT_Escape;
	ScanFunction[GLFW_KEY_F4] = KT_GoToEditMode;
	ScanFunction[GLFW_KEY_UP] = KT_Up;
	ScanFunction[GLFW_KEY_DOWN] = KT_Down;
	ScanFunction[GLFW_KEY_RIGHT] = KT_Right;
	ScanFunction[GLFW_KEY_LEFT] = KT_Left;
	ScanFunction[GLFW_KEY_SPACE] = KT_Select;
	ScanFunction[GLFW_KEY_ENTER] = KT_Enter;
	ScanFunction[GLFW_KEY_BACKSPACE] = KT_BSPC;
	ScanFunction[GLFW_MOUSE_BUTTON_LEFT] = KT_Select;
	ScanFunction[GLFW_MOUSE_BUTTON_RIGHT] = KT_SelectRight;
	ScanFunction['Z'] = KT_GameplayClick;
	ScanFunction['X'] = KT_GameplayClick;
	ScanFunction[GLFW_KEY_F1] = KT_FractionDec;
	ScanFunction[GLFW_KEY_F2] = KT_FractionInc;
	ScanFunction[GLFW_KEY_TAB] = KT_ChangeMode;
	ScanFunction[GLFW_KEY_F5] = KT_GridDec;
	ScanFunction[GLFW_KEY_F6] = KT_GridInc;
	ScanFunction[GLFW_KEY_F7] = KT_SwitchOffsetPrompt;
	ScanFunction[GLFW_KEY_F8] = KT_SwitchBPMPrompt;

	for (int i = 0; i < 16; i++)
	{
		char KString[256];
		sprintf(KString, "Key%d", i+1);

		char Binding = Configuration::GetConfigs(KString, "Keys7K").c_str()[0];
		
		if (Binding)
			ScanFunction7K[Binding] = (KeyType)(KT_Key1 + i);
	}
}

KeyType BindingsManager::TranslateKey(int32 Scan)
{
	if (ScanFunction.find(Scan) != ScanFunction.end())
	{
		return ScanFunction[Scan];
	}
	
	return KT_Unknown;
}

KeyType BindingsManager::TranslateKey7K(int32 Scan)
{
	if (ScanFunction7K.find(Scan) != ScanFunction7K.end())
	{
		return ScanFunction7K[Scan];
	}
	
	return KT_Unknown;
}



KeyEventType ToKeyEventType(int32 code)
{
	KeyEventType KE = KE_None;

	if (code == GLFW_PRESS)
		KE = KE_Press;
	else if (code == GLFW_RELEASE)
		KE = KE_Release;
	// Ignore GLFW_REPEAT events

	return KE;
}

GameWindow::GameWindow()
{
	Viewport.x = Viewport.y = 0;
	SizeRatio = 1.0f;
	FullscreenSwitchbackPending = false;
	wnd = NULL;
}

void ResizeFunc(GLFWwindow*, int32 width, int32 height)
{
	float HeightRatio = (float)height / WindowFrame.GetMatrixSize().y;

	WindowFrame.Viewport.x = width / 2 - WindowFrame.GetMatrixSize().x * HeightRatio / 2;
	WindowFrame.Viewport.y = 0;

	glViewport(WindowFrame.Viewport.x, 0, WindowFrame.GetMatrixSize().x * HeightRatio, height);

	WindowFrame.size.x = width;
	WindowFrame.size.y = height;

	WindowFrame.SizeRatio = HeightRatio;
}

void InputFunc (GLFWwindow*, int32 key, int32 scancode, int32 code, int32 modk)
{
	if (ToKeyEventType(code) != KE_None) // Ignore GLFW_REPEAT events
		App.HandleInput(key, ToKeyEventType(code), false);

	if (key == GLFW_KEY_ENTER && code == GLFW_PRESS && (modk & GLFW_MOD_ALT))
		WindowFrame.FullscreenSwitchbackPending = true;

	if (!WindowFrame.isGuiInputEnabled)
		return;
}

void MouseInputFunc (GLFWwindow*, int32 key, int32 code, int32 modk)
{
	if (ToKeyEventType(code) != KE_None) // Ignore GLFW_REPEAT events
		App.HandleInput(key, ToKeyEventType(code), true);

	if (!WindowFrame.isGuiInputEnabled)
		return;
}

void MouseMoveFunc (GLFWwindow*,double newx, double newy)
{
	if (!WindowFrame.isGuiInputEnabled)
		return;
}

glm::vec2 GameWindow::GetWindowSize() const
{
	return size;
}

glm::vec2 GameWindow::GetMatrixSize() const
{
	return matrixSize;
}

glm::vec2 GameWindow::GetRelativeMPos()
{
	double mousex, mousey;
	glfwGetCursorPos(wnd, &mousex, &mousey);
	float outx = (mousex - Viewport.x) / SizeRatio;
	float outy = matrixSize.y * mousey / size.y;
	return glm::vec2 (outx, outy);
}

void GameWindow::SetMatrix(uint32 MatrixMode, glm::mat4 matrix)
{
	glMatrixMode(MatrixMode);
	glLoadMatrixf((GLfloat*)&matrix[0]);
}

void GameWindow::SetupWindow()
{
	GLenum err;
	glfwMakeContextCurrent(wnd);

	// we have an opengl context, try opening up glew
	if ((err = glewInit()) != GLEW_OK)
	{
		std::stringstream serr;
		serr << "glew failed initialization: " << glewGetErrorString(err);
		throw; // std::exception(serr.str().c_str());
	}

	BindingsManager::Initialize();


	glEnable(GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CW);
	// glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	
	projection = glm::ortho<float>(0.0, matrixSize.x, matrixSize.y, 0.0);

	SetupShaders();

	// GLFW Hooks
	glfwSetFramebufferSizeCallback(wnd, ResizeFunc);
	glfwSetWindowSizeCallback(wnd, ResizeFunc);
	glfwSetKeyCallback(wnd, InputFunc);
	glfwSetMouseButtonCallback(wnd, MouseInputFunc);
	glfwSetCursorPosCallback(wnd, MouseMoveFunc);

	ResizeFunc(wnd, size.x, size.y);
}

void GameWindow::AutoSetupWindow()
{
	// todo: enum modes
	if (!glfwInit())
		throw; // std::exception("glfw failed initialization!"); // don't do shit

	AssignSize();
	matrixSize.x = ScreenWidth;
	matrixSize.y = ScreenHeight;

	IsFullscreen = Configuration::GetConfigf("Fullscreen") != 0;

#ifdef NDEBUG
	if (!(wnd = glfwCreateWindow(size.x, size.y, DOTCUR_WINDOWTITLE DOTCUR_VERSIONTEXT, IsFullscreen ? glfwGetPrimaryMonitor() : NULL, NULL)))
		throw; // std::exception("couldn't open window!");
#else
	if (!(wnd = glfwCreateWindow(size.x, size.y, DOTCUR_WINDOWTITLE DOTCUR_VERSIONTEXT, NULL, NULL)))
		throw; // std::exception("couldn't open window!");
#endif

	SetupWindow();
}

void GameWindow::AssignSize()
{
	float WindowWidth = Configuration::GetConfigf("WindowWidth");
	float WindowHeight = Configuration::GetConfigf("WindowHeight");

	if (WindowWidth == 0 || WindowHeight == 0)
	{
		GLFWmonitor *mon = glfwGetPrimaryMonitor();
		const GLFWvidmode *mode = glfwGetVideoMode(mon);

		size.x = mode->width;
		size.y = mode->height;
	}else
	{
		size.x = WindowWidth;
		size.y = WindowHeight;
	}
}

void GameWindow::SwapBuffers()
{
	glfwSwapBuffers(wnd);
	glfwPollEvents();

	/* Fullscreen switching */
	if (FullscreenSwitchbackPending)
	{
		if (IsFullscreen)
		{
			glfwDestroyWindow(wnd);
			wnd = glfwCreateWindow(size.x, size.y, DOTCUR_WINDOWTITLE DOTCUR_VERSIONTEXT, NULL, NULL);
		}else
		{
			AssignSize();

			glfwDestroyWindow(wnd);
			wnd = glfwCreateWindow(size.x, size.y, DOTCUR_WINDOWTITLE DOTCUR_VERSIONTEXT, glfwGetPrimaryMonitor(), NULL);
		}
		
		SetupWindow();
		ImageLoader::InvalidateAll();

		/* This revalidates all VBOs */
		for (std::vector<VBO*>::iterator i = VBOList.begin(); i != VBOList.end(); i++)
		{
			(*i)->Invalidate();
			(*i)->Validate();
		}

		IsFullscreen = !IsFullscreen;
		FullscreenSwitchbackPending = false;
	}

}

void GameWindow::ClearWindow()
{
	glClear(GL_COLOR_BUFFER_BIT);
}

int32 GameWindow::GetDefaultFragShader() const
{
	return defaultFragShader;
}

int32 GameWindow::GetDefaultVertexShader() const
{
	return defaultVertexShader;
}

int32 GameWindow::GetShaderProgram() const
{
	return defaultShaderProgram;
}

void GameWindow::Cleanup()
{
	glfwDestroyWindow(wnd);
	glfwTerminate();
}

bool GameWindow::ShouldCloseWindow()
{
	return !!glfwWindowShouldClose(wnd);
}

void GameWindow::SetVisibleCursor(bool Visible)
{
	if (Visible)
	{
		glfwSetInputMode(wnd, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}else
		glfwSetInputMode(wnd, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
}

void GameWindow::SetupShaders()
{

	/*
	glGenVertexArrays(1, &defaultVao);
	glBindVertexArray(defaultVao);
	*/

	defaultVertexShader = glCreateShader( GL_VERTEX_SHADER );
	glShaderSource( defaultVertexShader, 1, &vertShader, NULL );
	glCompileShader( defaultVertexShader );

	defaultFragShader = glCreateShader( GL_FRAGMENT_SHADER );
	glShaderSource( defaultFragShader, 1, &fragShader, NULL );
	glCompileShader( defaultFragShader );


	GLint status;
	char buffer[512];

	glGetShaderiv( defaultVertexShader, GL_COMPILE_STATUS, &status );
	
	glGetShaderInfoLog( defaultVertexShader, 512, NULL, buffer );

	if (status != GL_TRUE)
	{	
		throw; // std::exception(buffer);
	}else
		printf("%s\n", buffer);

	glGetShaderiv( defaultFragShader, GL_COMPILE_STATUS, &status );

	glGetShaderInfoLog( defaultFragShader, 512, NULL, buffer );

	if (status != GL_TRUE)
	{
		throw; // std::exception(buffer);
	}
	else printf("%s\n", buffer);

	defaultShaderProgram = glCreateProgram();
	glAttachShader( defaultShaderProgram, defaultVertexShader );
	glAttachShader( defaultShaderProgram, defaultFragShader );

	// glBindFragDataLocation( defaultShaderProgram, 0, "outColor" );

	glLinkProgram(defaultShaderProgram);

	// Use our recently compiled program.
	glUseProgram(defaultShaderProgram);
	
	// setup our projection matrix
	GLuint MatrixID = glGetUniformLocation(defaultShaderProgram, "projection");
	glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &projection[0][0]);

	// Clean up..
	glDeleteShader(defaultVertexShader);
	glDeleteShader(defaultFragShader);

	SetLightPosition(glm::vec3(0,0,1));
	SetLightMultiplier(1);
}

void GameWindow::SetUniform(String Uniform, int i) const
{
	GLint UniformID = glGetUniformLocation(defaultShaderProgram, Uniform.c_str());
	glUniform1i(UniformID, i);
}

void GameWindow::SetUniform(String Uniform, float A, float B, float C, float D) const
{
	GLint UniformID = glGetUniformLocation(defaultShaderProgram, Uniform.c_str());
	glUniform4f(UniformID, A, B, C, D);
}

void GameWindow::SetUniform(String Uniform, float *Matrix4x4) const
{
	GLint UniformID = glGetUniformLocation(defaultShaderProgram, Uniform.c_str());
	glUniformMatrix4fv(UniformID, 1, GL_FALSE, Matrix4x4);
}

int GameWindow::EnableAttribArray(String Attrib) const
{
	GLint AttribID = glGetAttribLocation(defaultShaderProgram, Attrib.c_str());
	glEnableVertexAttribArray(AttribID);
	return AttribID;
}

int GameWindow::DisableAttribArray(String Attrib) const
{
	GLint AttribID = glGetAttribLocation(defaultShaderProgram, Attrib.c_str());
	glDisableVertexAttribArray(AttribID);
	return AttribID;
}


void GameWindow::SetUniform(String Uniform, glm::vec3 Pos) const
{
	GLint UniformID = glGetUniformLocation(defaultShaderProgram, Uniform.c_str());
	glUniform3f(UniformID, Pos.x, Pos.y, Pos.z);
}

void GameWindow::SetUniform(String Uniform, float F) const
{
	GLint UniformID = glGetUniformLocation(defaultShaderProgram, Uniform.c_str());
	glUniform1f(UniformID, F);
}

void GameWindow::SetLightPosition(glm::vec3 Position) const
{
	SetUniform("lPos", Position);
}

void GameWindow::SetLightMultiplier(float Multiplier) const
{
	SetUniform("lMul", Multiplier);
}

void GameWindow::AddVBO(VBO *V)
{
	VBOList.push_back(V);
}

void GameWindow::RemoveVBO(VBO *V)
{
	for (std::vector<VBO*>::iterator i = VBOList.begin(); i != VBOList.end(); i++)
	{
		if (*i == V)
		{
			VBOList.erase(i);
			return;
		}
	}
}
