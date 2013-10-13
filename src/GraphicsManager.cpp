#include "Global.h"
#include "Screen.h"
#include "Application.h"
#include "GraphicsManager.h"
#include <glm/gtc/matrix_transform.hpp>

GraphicsManager GraphMan;

const char* vertShader = "#version 120\n"
	"attribute vec3 position;\n"
	"attribute vec2 vertexUV;\n"
	"uniform mat4 projection;\n"
	"uniform mat4 mvp;\n"
	"varying vec2 Texcoord;\n"
	"void main() {\n"
	"gl_Position = projection * mvp * vec4(position, 1);\n"
	"Texcoord = vertexUV;\n"
	"}\n";

const char* fragShader = "#version 120\n"
	"varying vec2 Texcoord;\n"
	"uniform vec4 Color;\n"
	"uniform sampler2D tex;\n"
	"uniform bool inverted;\n"
	"\n"
	"void main(void)\n"
	"{\n"
	"	 if (inverted) {\n"
	"		gl_FragColor = vec4(1 - texture2D(tex, Texcoord).r, 1 - texture2D(tex, Texcoord).g, 1 - texture2D(tex, Texcoord).b, texture2D(tex, Texcoord).a) * Color;\n"
	"	 }else{\n"
	"		gl_FragColor = texture2D(tex, Texcoord) * Color;\n"
	"	 }\n"
	"}\n";

void checkGlError()
{
	GLenum Val = glGetError();
	return; // breakpoints yay
}

GraphicsManager::GraphicsManager()
{
}

void GLFWCALL ResizeFunc(int32 width, int32 height)
{
	glViewport(width / 2 - GraphMan.GetMatrixSize().x / 2, 0, GraphMan.GetMatrixSize().x, height);
	GraphMan.size.x = width;
	GraphMan.size.y = height;
}

void GLFWCALL InputFunc (int32 key, int32 code)
{
	App.HandleInput(key, code, false);

	if (!GraphMan.isGuiInputEnabled)
		return;
}

void GLFWCALL MouseInputFunc (int32 key, int32 code)
{
	App.HandleInput(key, code, true);

	if (!GraphMan.isGuiInputEnabled)
		return;
}

void GLFWCALL MouseMoveFunc (int32 newx, int32 newy)
{
	if (!GraphMan.isGuiInputEnabled)
		return;
}

glm::vec2 GraphicsManager::GetWindowSize()
{
	return size;
}

glm::vec2 GraphicsManager::GetMatrixSize()
{
	return matrixSize;
}

glm::vec2 GraphicsManager::GetRelativeMPos()
{
	int32 mousex, mousey;
	glfwGetMousePos(&mousex, &mousey);
	float outx = matrixSize.x * mousex / size.x;
	float outy = matrixSize.y * mousey / size.y;
	return glm::vec2 (outx, outy);
}

void GraphicsManager::SetMatrix(GLenum MatrixMode, glm::mat4 matrix)
{
	glMatrixMode(MatrixMode);
	glLoadMatrixf((GLfloat*)&matrix[0]);
}

void GraphicsManager::AutoSetupWindow()
{
	GLenum err;
	GLFWvidmode mode;

	// todo: enum modes
	if (!glfwInit())
		throw std::exception("glfw failed initialization!"); // don't do shit

	size.x = ScreenWidth;
	size.y = ScreenHeight;
	matrixSize.x = ScreenWidth;
	matrixSize.y = ScreenHeight;

	glfwOpenWindowHint(GLFW_OPENGL_VERSION_MAJOR, 2);
	glfwOpenWindowHint(GLFW_OPENGL_VERSION_MINOR, 0);

	glfwGetDesktopMode(&mode);

	if (!glfwOpenWindow(size.x, size.y, mode.RedBits, mode.GreenBits, mode.BlueBits, 8, 16, 24, GLFW_WINDOW))
		throw std::exception("couldn't open window!");

	// we have an opengl context, try opening up glew
	if ((err = glewInit()) != GLEW_OK)
	{
		std::stringstream serr;
		serr << "glew failed initialization: " << glewGetErrorString(err);
		throw std::exception(serr.str().c_str());
	}

	glfwSetWindowTitle("dC Alpha"
#ifndef NDEBUG
		" (debug build)"
#endif
		);

	checkGlError();

#ifdef OLD_GL
	// We enable some GL stuff
	glEnable(GL_TEXTURE_2D);
	glDepthMask(GL_TRUE);
#endif

	glEnable(GL_BLEND);
	//glEnable(GL_COLOR_LOGIC_OP);
 	//glLogicOp(GL_COPY_INVERTED);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	// glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glViewport(0, 0, size.x, size.y);

	
	projection = glm::ortho<float>(0.0, matrixSize.x, matrixSize.y, 0.0, -32.0, 32.0);
#if (defined OLD_GL)
	SetMatrix(GL_PROJECTION, projection);
#endif

	SetupShaders();

	// GLFW Hooks
	glfwSetWindowSizeCallback(ResizeFunc);
	glfwSetKeyCallback(InputFunc);
	glfwSetMouseButtonCallback(MouseInputFunc);
	glfwSetMousePosCallback(MouseMoveFunc);

	// Don't show the cursor ;)
	//glfwDisable(GLFW_MOUSE_CURSOR);
	checkGlError();
}

void GraphicsManager::ClearWindow()
{
	glClear(GL_COLOR_BUFFER_BIT);
}

int32 GraphicsManager::GetDefaultFragShader()
{
	return defaultFragShader;
}

int32 GraphicsManager::GetDefaultVertexShader()
{
	return defaultVertexShader;
}

int32 GraphicsManager::GetShaderProgram()
{
	return defaultShaderProgram;
}

void GraphicsManager::SetupShaders()
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

	

	return;
#endif
}