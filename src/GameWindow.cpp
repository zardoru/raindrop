#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "Global.h"
#include "Screen.h"
#include "Application.h"
#include "GameWindow.h"
#include "ImageLoader.h"
#include "GraphObject2D.h"
#include "GameObject.h"
#include <glm/gtc/matrix_transform.hpp>


GameWindow WindowFrame;

const char* vertShader = "#version 120\n"
	"attribute vec3 position;\n"
	"attribute vec2 vertexUV;\n"
	"uniform mat4 projection;\n"
	"uniform mat4 mvp;\n"
	"varying vec2 Texcoord;\n"
	"void main() \n"
	"{\n"
	"	gl_Position = projection * mvp * vec4(position, 1);\n"
	"	Texcoord = vertexUV;\n"
	"}";

const char* fragShader = "#version 120\n"
	"varying vec2 Texcoord;\n"
	"uniform vec4 Color;\n"
	"uniform sampler2D tex;\n"
	"uniform bool inverted;\n"
	"\n"
	"void main(void)\n"
	"{\n"
	"	 if (inverted) {\n"
	"		gl_FragColor = vec4(1.0, 1.0, 1.0, 0.0) - texture2D(tex, Texcoord) * Color;\n"
	"	 }else{\n"
	"		gl_FragColor = texture2D(tex, Texcoord) * Color;\n"
	"	 }\n"
	"}\n";

std::map<int32, KeyType> BindingsManager::ScanFunction;

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
}

KeyType BindingsManager::TranslateKey(int32 Scan)
{
	if (ScanFunction.find(Scan) != ScanFunction.end())
	{
		return ScanFunction[Scan];
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
		throw std::exception(serr.str().c_str());
	}

	BindingsManager::Initialize();

#ifdef OLD_GL
	// We enable some GL stuff
	glEnable(GL_TEXTURE_2D);
	glDepthMask(GL_TRUE);
#endif

	glEnable(GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	// glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	
	projection = glm::ortho<float>(0.0, matrixSize.x, matrixSize.y, 0.0, -32.0, 32.0);
#if (defined OLD_GL)
	SetMatrix(GL_PROJECTION, projection);
#else
	SetupShaders();
#endif

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
		throw std::exception("glfw failed initialization!"); // don't do shit

	GLFWmonitor *mon = glfwGetPrimaryMonitor();
	const GLFWvidmode *mode = glfwGetVideoMode(mon);

	size.x = mode->width;
	size.y = mode->height;
	matrixSize.x = ScreenWidth;
	matrixSize.y = ScreenHeight;

	IsFullscreen = true;

#ifdef NDEBUG
	if (!(wnd = glfwCreateWindow(size.x, size.y, DOTCUR_WINDOWTITLE DOTCUR_VERSIONTEXT, glfwGetPrimaryMonitor(), NULL)))
		throw std::exception("couldn't open window!");
#else
	if (!(wnd = glfwCreateWindow(size.x, size.y, DOTCUR_WINDOWTITLE DOTCUR_VERSIONTEXT, NULL, NULL)))
		throw std::exception("couldn't open window!");
#endif

	SetupWindow();
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
			GLFWmonitor *mon = glfwGetPrimaryMonitor();
			const GLFWvidmode *mode = glfwGetVideoMode(mon);

			size.x = mode->width;
			size.y = mode->height;

			glfwDestroyWindow(wnd);
			wnd = glfwCreateWindow(size.x, size.y, DOTCUR_WINDOWTITLE DOTCUR_VERSIONTEXT, glfwGetPrimaryMonitor(), NULL);
		}
		
		SetupWindow();
		ImageLoader::InvalidateAll();

		/* This invalidates all GraphObject2Ds. */
		GraphObject2D GO1;
		GO1.Invalidate();
		GO1.Initialize(false);

		GameObject GO2;
		GO2.Invalidate();

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
#ifndef OLD_GL

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
		throw std::exception(buffer);
	}else
		printf("%s\n", buffer);

	glGetShaderiv( defaultFragShader, GL_COMPILE_STATUS, &status );

	glGetShaderInfoLog( defaultFragShader, 512, NULL, buffer );

	if (status != GL_TRUE)
	{
		throw std::exception(buffer);
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

#endif
}