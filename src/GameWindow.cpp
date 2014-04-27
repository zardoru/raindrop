#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <iostream>

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
	"uniform mat4 siM;\n"
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
	"		mat4 m = tranM;\n" // m = note position k = position
	"		m[3][1] *= sMult;\n"
	"		gl_Position = projection * m * mvp * siM * vec4(k_pos, 1);"
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
	"    vec4 tCol;\n"
	"	 vec4 tex2D = texture2D(tex, Texcoord);\n"
	"	 if (inverted) {\n"
	"		tCol = vec4(1.0 , 1.0, 1.0, tex2D.a*2) - tex2D * Color;\n"
	"	 }else{\n"
	"		tCol = tex2D * Color;\n"
	"	 }\n"
	"    float dist = length ( lPos - vec3(Pos_world.xy, 0) );\n"
	"    if (AffectedByLightning){\n"
	"       float temp = lMul / (dist*dist);\n"
	"		gl_FragColor = tCol * vec4(vec3(temp), 1);\n"
	"	 }else{\n"
	"		gl_FragColor = tCol;\n"
	"    }\n"
	"}\n";

std::map<int32, KeyType> BindingsManager::ScanFunction;
std::map<int32, KeyType> BindingsManager::ScanFunction7K;

struct sk_s
{
	const char* keystring;
	int boundkey;
};

sk_s SpecialKeys [] = // only add if someone actually needs more
{
	{"LShift", GLFW_KEY_LEFT_SHIFT},
	{"RShift", GLFW_KEY_RIGHT_SHIFT},
	{"Enter", GLFW_KEY_ENTER},
	{"LCtrl", GLFW_KEY_LEFT_CONTROL},
	{"RCtrl", GLFW_KEY_RIGHT_CONTROL},
	{"LAlt", GLFW_KEY_LEFT_ALT},
	{"RAlt", GLFW_KEY_RIGHT_ALT},
	{"Tab", GLFW_KEY_TAB},
	{"BSPC", GLFW_KEY_BACKSPACE}
};

int KeyTranslate(String K)
{
	for (int i = 0; i < (sizeof(SpecialKeys) / sizeof(sk_s)); i++)
	{
		if (K == SpecialKeys[i].keystring)
			return SpecialKeys[i].boundkey;
	}

	if (K.length())
	{
		if (Utility::IsNumeric(K.c_str()))
			return atoi(K.c_str());
		else
			return (int)K[0];
	}
	else
		return 0;
}

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

		int Binding = KeyTranslate(Configuration::GetConfigs(KString, "Keys7K"));
		
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

void ScrollFunc( GLFWwindow*, double xOff, double yOff )
{
	App.HandleScrollInput(xOff, yOff);
}

void MouseMoveFunc (GLFWwindow*,double newx, double newy)
{
	if (!WindowFrame.isGuiInputEnabled)
		return;
}

Vec2 GameWindow::GetWindowSize() const
{
	return size;
}

Vec2 GameWindow::GetMatrixSize() const
{
	return matrixSize;
}

Vec2 GameWindow::GetRelativeMPos()
{
	double mousex, mousey;
	glfwGetCursorPos(wnd, &mousex, &mousey);
	float outx = (mousex - Viewport.x) / SizeRatio;
	float outy = matrixSize.y * mousey / size.y;
	return Vec2 (outx, outy);
}

void GameWindow::SetMatrix(uint32 MatrixMode, Mat4 matrix)
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
	// glCullFace(GL_BACK);
	glFrontFace(GL_CW);
	// glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	
	projection = glm::ortho<float>(0.0, matrixSize.x, matrixSize.y, 0.0, -32.0, 0.0);

	SetupShaders();

	// GLFW Hooks
	glfwSetFramebufferSizeCallback(wnd, ResizeFunc);
	glfwSetWindowSizeCallback(wnd, ResizeFunc);
	glfwSetKeyCallback(wnd, InputFunc);
	glfwSetMouseButtonCallback(wnd, MouseInputFunc);
	glfwSetCursorPosCallback(wnd, MouseMoveFunc);
	glfwSetScrollCallback(wnd, ScrollFunc);

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

	if (!(wnd = glfwCreateWindow(size.x, size.y, DOTCUR_WINDOWTITLE DOTCUR_VERSIONTEXT, IsFullscreen ? glfwGetPrimaryMonitor() : NULL, NULL)))
		throw; // std::exception("couldn't open window!");

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
		
		AttribLocs.clear();
		UniformLocs.clear();
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
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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
		std::wcout << buffer << "\n";
		throw; // std::exception(buffer);
	}else
		std::wcout << buffer << "\n";

	glGetShaderiv( defaultFragShader, GL_COMPILE_STATUS, &status );

	glGetShaderInfoLog( defaultFragShader, 512, NULL, buffer );

	if (status != GL_TRUE)
	{
		std::wcout << buffer << "\n";
		throw; // std::exception(buffer);
	}
	else std::wcout << buffer << "\n";

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

	uniforms[A_POSITION] = glGetAttribLocation(defaultShaderProgram, "position");
	uniforms[A_UV] = glGetAttribLocation(defaultShaderProgram, "vertexUV");
	uniforms[U_TRANM] = glGetUniformLocation(defaultShaderProgram, "tranM");
	uniforms[U_MVP] = glGetUniformLocation(defaultShaderProgram, "mvp");
	uniforms[U_SIM] = glGetUniformLocation(defaultShaderProgram, "siM");
	uniforms[U_TRANSL] = glGetUniformLocation(defaultShaderProgram, "useTranslate");
	uniforms[U_SMULT] = glGetUniformLocation(defaultShaderProgram, "sMult");
	uniforms[U_CENTERED] = glGetUniformLocation(defaultShaderProgram, "Centered");
	uniforms[U_COLOR] = glGetUniformLocation(defaultShaderProgram, "Color");
	uniforms[U_LMUL] = glGetUniformLocation(defaultShaderProgram, "lMul");
	uniforms[U_LPOS] = glGetUniformLocation(defaultShaderProgram, "lPos");
	uniforms[U_INVERT] = glGetUniformLocation(defaultShaderProgram, "inverted");
	uniforms[U_LIGHT] = glGetUniformLocation(defaultShaderProgram, "AffectedByLightning");

	SetLightPosition(glm::vec3(0,0,1));
	SetLightMultiplier(1);
}

void GameWindow::SetUniform(uint32 Uniform, int i) 
{
	glUniform1i(uniforms[Uniform], i);
}

void GameWindow::SetUniform(uint32 Uniform, float A, float B, float C, float D) 
{
	glUniform4f(uniforms[Uniform], A, B, C, D);
}



void GameWindow::SetUniform(uint32 Uniform, glm::vec3 Pos) 
{
	glUniform3f(uniforms[Uniform], Pos.x, Pos.y, Pos.z);
}

void GameWindow::SetUniform(uint32 Uniform, float F) 
{
	glUniform1f(uniforms[Uniform], F);
}

void GameWindow::SetUniform(uint32 Uniform, float *Matrix4x4) 
{
	glUniformMatrix4fv(uniforms[Uniform], 1, GL_FALSE, Matrix4x4);
}

int GameWindow::EnableAttribArray(uint32 Attrib)
{
	glEnableVertexAttribArray(uniforms[Attrib]);
	return uniforms[Attrib];
}

int GameWindow::DisableAttribArray(uint32 Attrib) 
{
	glDisableVertexAttribArray(uniforms[Attrib]);
	return uniforms[Attrib];
}


void GameWindow::SetLightPosition(glm::vec3 Position) 
{
	SetUniform(U_LPOS, Position);
}

void GameWindow::SetLightMultiplier(float Multiplier) 
{
	SetUniform(U_LMUL, Multiplier);
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
