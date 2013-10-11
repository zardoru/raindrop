#include "Global.h"
#include "Screen.h"
#include "Application.h"
#include "GraphicsManager.h"
#include <glm/gtc/matrix_transform.hpp>  
#include <CEGUI.h>
#include <RendererModules/OpenGL/CEGUIOpenGLRenderer.h>

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

uint32 GlfwToCeguiKey(int32 glfwKey)
{
	switch(glfwKey)
	{
		case GLFW_KEY_UNKNOWN	: return 0;
		case GLFW_KEY_ESC	: return CEGUI::Key::Escape;
		case GLFW_KEY_F1	: return CEGUI::Key::F1;
		case GLFW_KEY_F2	: return CEGUI::Key::F2;
		case GLFW_KEY_F3	: return CEGUI::Key::F3;
		case GLFW_KEY_F4	: return CEGUI::Key::F4;
		case GLFW_KEY_F5	: return CEGUI::Key::F5;
		case GLFW_KEY_F6	: return CEGUI::Key::F6;
		case GLFW_KEY_F7	: return CEGUI::Key::F7;
		case GLFW_KEY_F8	: return CEGUI::Key::F8;
		case GLFW_KEY_F9	: return CEGUI::Key::F9;
		case GLFW_KEY_F10       : return CEGUI::Key::F10;
		case GLFW_KEY_F11       : return CEGUI::Key::F11;
		case GLFW_KEY_F12       : return CEGUI::Key::F12;
		case GLFW_KEY_F13       : return CEGUI::Key::F13;
		case GLFW_KEY_F14       : return CEGUI::Key::F14;
		case GLFW_KEY_F15       : return CEGUI::Key::F15;
		case GLFW_KEY_UP        : return CEGUI::Key::ArrowUp;
		case GLFW_KEY_DOWN      : return CEGUI::Key::ArrowDown;
		case GLFW_KEY_LEFT      : return CEGUI::Key::ArrowLeft;
		case GLFW_KEY_RIGHT     : return CEGUI::Key::ArrowRight;
		case GLFW_KEY_LSHIFT    : return CEGUI::Key::LeftShift;
		case GLFW_KEY_RSHIFT    : return CEGUI::Key::RightShift;
		case GLFW_KEY_LCTRL     : return CEGUI::Key::LeftControl;
		case GLFW_KEY_RCTRL     : return CEGUI::Key::RightControl;
		case GLFW_KEY_LALT      : return CEGUI::Key::LeftAlt;
		case GLFW_KEY_RALT      : return CEGUI::Key::RightAlt;
		case GLFW_KEY_TAB       : return CEGUI::Key::Tab;
		case GLFW_KEY_ENTER     : return CEGUI::Key::Return;
		case GLFW_KEY_BACKSPACE : return CEGUI::Key::Backspace;
		case GLFW_KEY_INSERT    : return CEGUI::Key::Insert;
		case GLFW_KEY_DEL       : return CEGUI::Key::Delete;
		case GLFW_KEY_PAGEUP    : return CEGUI::Key::PageUp;
		case GLFW_KEY_PAGEDOWN  : return CEGUI::Key::PageDown;
		case GLFW_KEY_HOME      : return CEGUI::Key::Home;
		case GLFW_KEY_END       : return CEGUI::Key::End;
		case GLFW_KEY_KP_ENTER	: return CEGUI::Key::NumpadEnter;
		default			: return 0;
	}
}

CEGUI::MouseButton GlfwToCeguiButton(int32 glfwButton)
{
	switch(glfwButton)
	{
		case GLFW_MOUSE_BUTTON_LEFT	: return CEGUI::LeftButton;
		case GLFW_MOUSE_BUTTON_RIGHT	: return CEGUI::RightButton;
		case GLFW_MOUSE_BUTTON_MIDDLE	: return CEGUI::MiddleButton;
		default				: return CEGUI::NoButton;
	}
}

GraphicsManager::GraphicsManager()
{
}

void GLFWCALL ResizeFunc(int32 width, int32 height)
{
	glViewport(width / 2 - GraphMan.GetMatrixSize().x / 2, 0, GraphMan.GetMatrixSize().x, height);
	GraphMan.size.x = width;
	GraphMan.size.y = height;

#ifndef DISABLE_CEGUI
	CEGUI::System::getSingleton().notifyDisplaySizeChanged(CEGUI::Size(width, height));
#endif
}

void GLFWCALL InputFunc (int32 key, int32 code)
{
	App.HandleInput(key, code, false);

	if (!GraphMan.isGuiInputEnabled)
		return;

#ifndef DISABLE_CEGUI
	if (code == GLFW_PRESS)
	{
		if (GlfwToCeguiKey(key) == 0)
			CEGUI::System::getSingleton().injectChar(key);
		else
			CEGUI::System::getSingleton().injectKeyDown(GlfwToCeguiKey(key));
	}
	else
		CEGUI::System::getSingleton().injectKeyUp(key);
#endif
}

void GLFWCALL MouseInputFunc (int32 key, int32 code)
{
	App.HandleInput(key, code, true);

	if (!GraphMan.isGuiInputEnabled)
		return;
#ifndef DISABLE_CEGUI
	if (code == GLFW_PRESS)
	{
		CEGUI::System::getSingleton().injectMouseButtonDown(GlfwToCeguiButton(key));
	}
	else
		CEGUI::System::getSingleton().injectMouseButtonUp(GlfwToCeguiButton(key));
#endif
}

void GLFWCALL MouseMoveFunc (int32 newx, int32 newy)
{
	if (!GraphMan.isGuiInputEnabled)
		return;

#ifndef DISABLE_CEGUI
	CEGUI::System::getSingleton().injectMousePosition(newx, newy);
#endif
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

	glfwOpenWindowHint(GLFW_OPENGL_VERSION_MAJOR, 1);
	glfwOpenWindowHint(GLFW_OPENGL_VERSION_MINOR, 2);

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
	SetupCEGUI();

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

void GraphicsManager::SetupCEGUI()
{
#ifndef DISABLE_CEGUI
	CEGUI::OpenGLRenderer& myRenderer = CEGUI::OpenGLRenderer::bootstrapSystem();

	CEGUI::DefaultResourceProvider* rp =
        static_cast<CEGUI::DefaultResourceProvider*>
            (CEGUI::System::getSingleton().getResourceProvider());
    
    const char* dataPathPrefix = "./GameData/GUI/";

	// az note: i literally c&p shit from CEGUI's samples because holy shit what
    // for each resource type, set a resource group directory
    rp->setResourceGroupDirectory("schemes", dataPathPrefix);
    rp->setResourceGroupDirectory("imagesets", dataPathPrefix);
    rp->setResourceGroupDirectory("fonts", dataPathPrefix);
    rp->setResourceGroupDirectory("layouts", dataPathPrefix);
    rp->setResourceGroupDirectory("looknfeels", dataPathPrefix);
    rp->setResourceGroupDirectory("schemas", dataPathPrefix);
    rp->setResourceGroupDirectory("animations", dataPathPrefix);

	    // set the default resource groups to be used
    CEGUI::Imageset::setDefaultResourceGroup("imagesets");
    CEGUI::Font::setDefaultResourceGroup("fonts");
    CEGUI::Scheme::setDefaultResourceGroup("schemes");
    CEGUI::WidgetLookManager::setDefaultResourceGroup("looknfeels");
    CEGUI::WindowManager::setDefaultResourceGroup("layouts");
    CEGUI::ScriptModule::setDefaultResourceGroup("lua_scripts");
    CEGUI::AnimationManager::setDefaultResourceGroup("animations");
    
    // setup default group for validation schemas
    CEGUI::XMLParser* parser = CEGUI::System::getSingleton().getXMLParser();
    if (parser->isPropertyPresent("SchemaDefaultResourceGroup"))
        parser->setProperty("SchemaDefaultResourceGroup", "schemas");     

	// create (load) the TaharezLook scheme file
	// (this auto-loads the TaharezLook looknfeel and imageset files)
	CEGUI::SchemeManager::getSingleton().create( "TaharezLook.scheme" );

	// create (load) a font.
	// The first font loaded automatically becomes the default font, but note
	// that the scheme might have already loaded a font, so there may already
	// be a default set - if we want the "Commonweath-10" font to definitely
	// be the default, we should set the default explicitly afterwards.
	CEGUI::FontManager::getSingleton().create( "DejaVuSans-10.font" );

	CEGUI::System::getSingleton().setDefaultMouseCursor("TaharezLook", "MouseArrow");
	CEGUI::System::getSingleton().setMouseClickEventGenerationEnabled(false);
	CEGUI::System::getSingleton().setDefaultTooltip("TaharezLook/Tooltip");
#endif
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