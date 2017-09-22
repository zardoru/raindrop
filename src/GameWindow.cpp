#include "pch.h"

#include "GameGlobal.h"
#include "Logging.h"
#include "BindingsManager.h"

#include "Screen.h"
#include "Application.h"
#include "GameWindow.h"
#include "ImageLoader.h"
#include "Sprite.h"
#include "VBO.h"
#include "TruetypeFont.h"
#include "RaindropRocketInterface.h"

#include "Shader.h"
//#include <glm/gtc/matrix_transform.hpp>

GameWindow WindowFrame;

std::map<int32_t, KeyType> BindingsManager::ScanFunction;
std::map<int32_t, int32_t> BindingsManager::ScanFunction7K;

const int NUM_OF_USED_CONTROLLER_BUTTONS = 32;

int controllerToUse;
bool JoystickEnabled;

// az: wait - this is kind of a bad idea (limited size array)
// TODO: there's ought to be a better way to do this

//True is pressed, false is released
bool controllerButtonState[NUM_OF_USED_CONTROLLER_BUTTONS + 1] = { 0 };
float lastAxisSign[NUM_OF_USED_CONTROLLER_BUTTONS + 1] = { 0 };
float lastAxisValue[NUM_OF_USED_CONTROLLER_BUTTONS + 1] = { 0 };
//The first member of this array should never be accessed; it's there to make reading some of the code easier.

struct KeyAssociation
{
    char KeyString[32];
    int boundkey;
};

KeyAssociation StaticSpecialKeys[] = // only add if someone actually needs more
{
    { "LShift", GLFW_KEY_LEFT_SHIFT },
    { "RShift", GLFW_KEY_RIGHT_SHIFT },
    { "Enter", GLFW_KEY_ENTER },
    { "LCtrl", GLFW_KEY_LEFT_CONTROL },
    { "RCtrl", GLFW_KEY_RIGHT_CONTROL },
    { "LAlt", GLFW_KEY_LEFT_ALT },
    { "RAlt", GLFW_KEY_RIGHT_ALT },
    { "Tab", GLFW_KEY_TAB },
    { "BSPC", GLFW_KEY_BACKSPACE },
    { "F1", GLFW_KEY_F1 },
    { "F2", GLFW_KEY_F2 },
    { "F3", GLFW_KEY_F3 },
    { "F4", GLFW_KEY_F4 },
    { "F5", GLFW_KEY_F5 },
    { "F6", GLFW_KEY_F6 },
    { "F7", GLFW_KEY_F7 },
    { "F8", GLFW_KEY_F8 },
    { "F9", GLFW_KEY_F9 },
    { "F10", GLFW_KEY_F10 },
    { "F11", GLFW_KEY_F11 },
    { "F12", GLFW_KEY_F12 },
    { "Supr", GLFW_KEY_DELETE },
    { "End", GLFW_KEY_END },
    { "Home", GLFW_KEY_HOME },
    { "Insert", GLFW_KEY_INSERT },
    { "PrintScreen", GLFW_KEY_PRINT_SCREEN },
    { "PageDown", GLFW_KEY_PAGE_DOWN },
    { "PageUp", GLFW_KEY_PAGE_UP },
    { "Pause", GLFW_KEY_PAUSE },
    { "Escape", GLFW_KEY_ESCAPE },
    { "UpArrow", GLFW_KEY_UP },
    { "DownArrow", GLFW_KEY_DOWN },
    { "LeftArrow", GLFW_KEY_LEFT },
    { "RightArrow", GLFW_KEY_RIGHT },
    { "Space", GLFW_KEY_SPACE },
    { "Enter", GLFW_KEY_ENTER },
    { "Backspace", GLFW_KEY_BACKSPACE }
};

const int NUM_OF_STATIC_SPECIAL_KEYS = sizeof(StaticSpecialKeys) / sizeof(KeyAssociation); //make sure to match the above array

std::vector<KeyAssociation> SpecialKeys;

int KeyTranslate(std::string K)
{
    for (auto i = 0U; i < SpecialKeys.size(); i++)
    {
        std::string Key = K; Utility::ToLower(Key);
        std::string Target = std::string(SpecialKeys.at(i).KeyString);  Utility::ToLower(Target);
        if (Key == Target)
            return SpecialKeys.at(i).boundkey;
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

struct defaultKeys_s
{
    int key;
    KeyType command;
} defaultKeys[] = {
    { GLFW_KEY_ESCAPE, KT_Escape },
    { GLFW_KEY_UP, KT_Up },
    { GLFW_KEY_DOWN, KT_Down },
    { GLFW_KEY_LEFT, KT_Left },
    { GLFW_KEY_RIGHT, KT_Right },
    { GLFW_KEY_SPACE, KT_Select },
    { GLFW_KEY_ENTER, KT_Enter },
    { GLFW_KEY_BACKSPACE, KT_BSPC },
    { GLFW_MOUSE_BUTTON_LEFT, KT_Select },
    { GLFW_MOUSE_BUTTON_RIGHT, KT_SelectRight },
	{ GLFW_KEY_F5, KT_ReloadScreenScripts },
	{ GLFW_KEY_F10, KT_ReloadCFG }
};

const int DEFAULT_KEYS_COUNT = sizeof(defaultKeys) / sizeof(defaultKeys_s);

// Must match KeyType structure.
const char* KeytypeNames[] = {
    "unknown",
    "escape",
    "select",
    "enter",
    "bspc",
    "select2",
    "up",
    "down",
    "left",
    "right",
    "reload",
    "debug",
	"reloadconfig",
    "hit",
    "edfracdecrease",
	"edfracincrease",
	"edmode",
	"edsetoffset",
	"scratchp1up",
	"scratchp1down",
	"scratchp2up",
	"scratchp2down",
};

int getIndexForKeytype(const char* key)
{
    for (int i = 0; i < sizeof KeytypeNames / sizeof(char*); i++)
    {
        std::string lowkey = std::string(key); Utility::ToLower(lowkey);
        std::string lowname = std::string(KeytypeNames[i]); Utility::ToLower(lowname);
        if (lowkey == lowname)
            return i;
    }

    return -1;
}

std::string getNameForKeytype(KeyType K)
{
    if (K < sizeof KeytypeNames / sizeof(char*))
        return KeytypeNames[K];
    else
        return Utility::IntToStr(K);
}

std::string getNameForUntranslatedKey(int K)
{
    for (int i = 0; i < NUM_OF_STATIC_SPECIAL_KEYS; i++)
    {
        if (StaticSpecialKeys[i].boundkey == K)
            return StaticSpecialKeys[i].KeyString;
    }

    return Utility::IntToStr(K);
}

void BindingsManager::Initialize()
{
    SpecialKeys.clear();
    for (int i = 0; i < NUM_OF_STATIC_SPECIAL_KEYS; i++)
        SpecialKeys.push_back(StaticSpecialKeys[i]);

    //controllerToUse = 1; should use this if the user entered garbage data (anything that isn't a number)
    controllerToUse = (int)Configuration::GetConfigf("ControllerNumber") - 1;

    if (glfwJoystickPresent(controllerToUse))
    {
        int numOfButtons;
        glfwGetJoystickButtons(controllerToUse, &numOfButtons);
        if (numOfButtons)
        {
            for (int i = 1; i <= numOfButtons; i++)
            {
                char name[32];
                sprintf(name, "Controller%d", i);
                KeyAssociation thisButton;
                strcpy(thisButton.KeyString, name);
                thisButton.boundkey = 1000 + i;
                SpecialKeys.push_back(thisButton);
            }
        }

		int numOfAxis;
		glfwGetJoystickAxes(controllerToUse, &numOfAxis);
		if (numOfAxis)
		{
			for (int i = numOfButtons + 1; i <= numOfButtons + numOfAxis; i++) {
				char name[32];
				sprintf(name, "Controller%d", i);
				KeyAssociation thisAxis;
				strcpy(thisAxis.KeyString, name);
				thisAxis.boundkey = 1000 + i;
				SpecialKeys.push_back(thisAxis);
			}
		}
    }

    JoystickEnabled = (glfwJoystickPresent(GLFW_JOYSTICK_1) == GL_TRUE);

    std::map <std::string, std::string> fields;
    Configuration::GetConfigListS("SystemKeys", fields, "");

    // key = function
    // e.g. Z = gameclick, X = gameclick
    for (auto i = fields.begin(); i != fields.end(); i++)
    {
        // transform special name into keytype index
        int idx = getIndexForKeytype(i->second.c_str());

        // ah it's valid
        if (idx != -1)
        {
            // get the key in either int or name or char format and save that into the key -> command translator
            int Key = KeyTranslate(i->first.c_str());

            if (Key) // a valid key, probably
                ScanFunction[Key] = (KeyType)idx;
        }
    }

    // fill missing default keys after it's done
    for (int i = 0; i < DEFAULT_KEYS_COUNT; i++)
    {
        if (ScanFunction.find(defaultKeys[i].key) == ScanFunction.end())
        {
            // fill the key -> command translation
            ScanFunction[defaultKeys[i].key] = defaultKeys[i].command;

            // write it out to the config file
            std::string charOut;
            if (defaultKeys[i].key <= 255 && isgraph(defaultKeys[i].key)) // we're not setting like, gibberish
            {
                charOut = Utility::CharToStr(defaultKeys[i].key);
            }
            else
            {
                charOut = getNameForUntranslatedKey(defaultKeys[i].key);
            }

            Configuration::SetConfig(charOut, getNameForKeytype(defaultKeys[i].command), "SystemKeys");
        }
    }

    int i = 1;
    std::map<std::string, std::string> Keys;
    Configuration::GetConfigListS("Keys7K", Keys, "");

    for (auto v : Keys)
    {
        int Binding = KeyTranslate(v.first);
        if (Binding)
            ScanFunction7K[Binding] = floor(latof(v.second));
    }
}

KeyType BindingsManager::TranslateKey(int32_t Scan)
{
    if (ScanFunction.find(Scan) != ScanFunction.end())
    {
        return ScanFunction[Scan];
    }

    return KT_Unknown;
}

int32_t BindingsManager::TranslateKey7K(int32_t Scan)
{
    if (ScanFunction7K.find(Scan) != ScanFunction7K.end())
    {
        return ScanFunction7K[Scan];
    }

    return -1;
}

KeyEventType ToKeyEventType(int32_t code)
{
    KeyEventType KE = KE_NONE;

    if (code == GLFW_PRESS)
        KE = KE_PRESS;
    else if (code == GLFW_RELEASE)
        KE = KE_RELEASE;
    // Ignore GLFW_REPEAT events

    return KE;
}

bool doFlush = false;
bool VSync = false;

GameWindow::GameWindow()
{
    Viewport.x = Viewport.y = 0;
    SizeRatio = 1.0f;
    FullscreenSwitchbackPending = false;
    wnd = NULL;
}

void ResizeFunc(GLFWwindow* wnd, int32_t width, int32_t height)
{
    float HeightRatio = (float)height / WindowFrame.GetMatrixSize().y;

	if (!WindowFrame.IsFullscreen) { // well then, let's enforce some aspect ratio
		double mwidth = WindowFrame.GetMatrixSize().x * HeightRatio;
		glViewport(0, 0, mwidth, height);
		glfwSetWindowSize(wnd, mwidth, height);

		WindowFrame.size.x = mwidth;
		WindowFrame.size.y = height;
	}
	else { // just assume the values are correct in fullscreen
		glViewport(0, 0, width, height);
		WindowFrame.size.x = width;
		WindowFrame.size.y = height;
	}

    WindowFrame.SizeRatio = HeightRatio;
}

void InputFunc(GLFWwindow*, int32_t key, int32_t scancode, int32_t code, int32_t modk)
{
    if (ToKeyEventType(code) != KE_NONE) // Ignore GLFW_REPEAT events
        WindowFrame.Parent->HandleInput(key, ToKeyEventType(code), false);

    if (key == GLFW_KEY_ENTER && code == GLFW_PRESS && (modk & GLFW_MOD_ALT))
        WindowFrame.FullscreenSwitchbackPending = true;
}

void MouseInputFunc(GLFWwindow*, int32_t key, int32_t code, int32_t modk)
{
    if (ToKeyEventType(code) != KE_NONE) // Ignore GLFW_REPEAT events
        WindowFrame.Parent->HandleInput(key, ToKeyEventType(code), true);
}

void ScrollFunc(GLFWwindow*, double xOff, double yOff)
{
    WindowFrame.Parent->HandleScrollInput(xOff, yOff);
}

void MouseMoveFunc(GLFWwindow*, double newx, double newy)
{
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
    return Vec2(outx, outy);
}

Vec2 GameWindow::GetWindowMPos()
{
    double mousex, mousey;
    glfwGetCursorPos(wnd, &mousex, &mousey);
    return Vec2(mousex, mousey);
}

float GameWindow::GetWindowVScale()
{
    return SizeRatio;
}

void CharInputFunc(GLFWwindow*, unsigned int cp)
{
    WindowFrame.Parent->HandleTextInput(cp);
}

bool GameWindow::SetupWindow()
{
    GLenum err;
    glfwMakeContextCurrent(wnd);

    // we have an opengl context, try opening up glew
    if ((err = glewInit()) != GLEW_OK)
    {
        Log::Logf("glew failed initialization: %s", glewGetErrorString(err));
        return false;
    }

    BindingsManager::Initialize();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // glEnable(GL_CULL_FACE);
    glEnable(GL_LINE_SMOOTH);
    // glCullFace(GL_BACK);
    // glFrontFace(GL_CW);
    // glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

	//glRenderbufferStorage(GL_RENDERBUFFER, )
	glEnable(GL_FRAMEBUFFER_SRGB);

    glEnable(GL_ALPHA_TEST);
	glEnable(GL_POLYGON_SMOOTH);
    glAlphaFunc(GL_GREATER, 0);

    if (VSync)
        glfwSwapInterval(1);

    projection = glm::ortho<float>(0.0, matrixSize.x, matrixSize.y, 0.0, -32.0, 0.0);
    projectionInverse = glm::inverse(projection);

    if (!SetupShaders())
        return false;

    Renderer::InitializeRender();

    // GLFW Hooks
    glfwSetFramebufferSizeCallback(wnd, ResizeFunc);
    glfwSetWindowSizeCallback(wnd, ResizeFunc);
    glfwSetKeyCallback(wnd, InputFunc);
    glfwSetMouseButtonCallback(wnd, MouseInputFunc);
    glfwSetCursorPosCallback(wnd, MouseMoveFunc);
    glfwSetScrollCallback(wnd, ScrollFunc);
    glfwSetCharCallback(wnd, CharInputFunc);

    ResizeFunc(wnd, size.x, size.y);

    return true;
}

Mat4 GameWindow::GetMatrixProjection()
{
    return projection;
}

Mat4 GameWindow::GetMatrixProjectionInverse()
{
    return projectionInverse;
}

void glfwError(int c, const char* s)
{
    Log::LogPrintf("GLFW Error %d: %s\n", c, s);
}

bool GameWindow::AutoSetupWindow(Application* _parent)
{
    Parent = _parent;

    glfwSetErrorCallback(glfwError);

    // todo: enum modes
    if (!glfwInit())
    {
        Log::LogPrintf("Failure to initialize glfw.\n");
        return false; // std::exception("glfw failed initialization!"); // don't do shit
	}
	else {
		Log::LogPrintf("GLFW succesfully initialized.\n");
	}

	glfwWindowHint(GLFW_SRGB_CAPABLE, 1);

    AssignSize();
    matrixSize.x = ScreenWidth;
    matrixSize.y = ScreenHeight;

    IsFullscreen = Configuration::GetConfigf("Fullscreen") != 0;

    doFlush = Configuration::GetConfigf("VideoFlush") != 0;
    VSync = Configuration::GetConfigf("VSync") != 0;

	if (IsFullscreen) {
		if (!glfwGetPrimaryMonitor()) {
			Log::LogPrintf("Can't get primary window (Fullscreen)\n");
			IsFullscreen = false;
		}
	}

    if (!(wnd = glfwCreateWindow(size.x, size.y, RAINDROP_WINDOWTITLE RAINDROP_VERSIONTEXT, IsFullscreen ? glfwGetPrimaryMonitor() : NULL, NULL)))
    {
        Log::Logf("Failure to initialize window.\n");
        return false;
    }

#ifdef DARWIN
    // This is a temporary hack for OS X where our size isn't getting initialized to the correct values.
    int outx = 0;
    int outy = 0;
    glfwGetWindowSize(wnd, &outx, &outy);
    ResizeFunc(wnd, outx, outy);
#endif

    SetVisibleCursor(Configuration::GetSkinConfigf("ShowCursor") != 0);

    return SetupWindow();
}

void GameWindow::AssignSize()
{
    float WindowWidth = Configuration::GetConfigf("WindowWidth");
    float WindowHeight = Configuration::GetConfigf("WindowHeight");

    if (WindowWidth == 0 || WindowHeight == 0)
    {
        GLFWmonitor *mon = glfwGetPrimaryMonitor();

		if (mon) {
			const GLFWvidmode *mode = glfwGetVideoMode(mon);

			size.x = mode->width;
			size.y = mode->height;
		}
		else {
			Log::LogPrintf("Monitor == null? Defaulting to 1024x768.");
			WindowWidth = 1024;
			WindowHeight = 768;
			goto autosize;
		}
    }
    else
    {
		autosize:
        size.x = WindowWidth;
        size.y = WindowHeight;
    }
}

void GameWindow::SwapBuffers()
{
	if (doFlush)
		glFlush();

	glfwSwapBuffers(wnd);
	

    /* Fullscreen switching */

}

void GameWindow::ClearWindow()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void GameWindow::Cleanup()
{
	glfwDestroyWindow(wnd);
	glfwTerminate();
}

void GameWindow::UpdateFullscreen()
{
	if (FullscreenSwitchbackPending)
	{
		Log::LogPrintf("Attempting to switch fullscreen mode.\n");
		if (IsFullscreen)
		{
			glfwDestroyWindow(wnd);
			wnd = glfwCreateWindow(size.x, size.y, RAINDROP_WINDOWTITLE RAINDROP_VERSIONTEXT, NULL, NULL);
			IsFullscreen = false;
		}
		else
		{
			if (glfwGetPrimaryMonitor()) {
				AssignSize();
				glfwDestroyWindow(wnd);
				wnd = glfwCreateWindow(size.x, size.y, RAINDROP_WINDOWTITLE RAINDROP_VERSIONTEXT, glfwGetPrimaryMonitor(), NULL);

				IsFullscreen = true;
				Log::LogPrintf("Switched to fullscreen mode.\n");
			}
			else {
				IsFullscreen = false;
				Log::LogPrintf("Can't switch to fullscreen. No primary monitor detected?\n");
				FullscreenSwitchbackPending = false;
				return;
			}
		}

		SetupWindow();

		// Reload all images.
		Engine::RocketInterface::ReloadTextures();
		ImageLoader::ReloadAll();

		/* This revalidates all VBOs and fonts */
		for (std::vector<VBO*>::iterator i = VBOList.begin(); i != VBOList.end(); i++)
		{
			(*i)->Invalidate();
			(*i)->Validate();
		}

		// Automatically revalidated on usage
		for (std::vector<TruetypeFont*>::iterator i = TTFList.begin(); i != TTFList.end(); i++)
		{
			(*i)->Invalidate();
		}

		FullscreenSwitchbackPending = false;
	}
}

void GameWindow::RunInput()
{
	glfwPollEvents();

	if (JoystickEnabled)
	{
		// buttons
		int buttonArraySize = 0;
		const unsigned char *buttonArray = glfwGetJoystickButtons(GLFW_JOYSTICK_1, &buttonArraySize);
		if (buttonArraySize > 0)
		{
			for (int i = 0; i < buttonArraySize; i++)
			{
				for (uint32_t j = 0; j < SpecialKeys.size(); j++)
				{
					/* Matches the pressed button to its entry in the SpecialKeys vector. */
					int thisKeyNumber = SpecialKeys[j].boundkey - 1000;
					if (i + 1 == thisKeyNumber)
					{
						/* Only processes the button push/release if the state has changed. */
						if ((buttonArray[i] != 0) != controllerButtonState[thisKeyNumber])
						{
							WindowFrame.Parent->HandleInput(SpecialKeys[j].boundkey, ToKeyEventType(buttonArray[i]), false);
							controllerButtonState[thisKeyNumber] = !controllerButtonState[thisKeyNumber];
						}
					}
				}
			}
		}

		// axis
		int axisArraySize;
		float deadzone = 0.25;
		const float *axisArray = glfwGetJoystickAxes(GLFW_JOYSTICK_1, &axisArraySize);
		if (axisArraySize) {
			for (auto i = 0; i < axisArraySize; i++) {
				for (size_t j = 0; j < SpecialKeys.size(); j++) {
					// as before, specialkeys vector value
					int axis = SpecialKeys[j].boundkey - 1000;

					if ((i + buttonArraySize + 1) != axis)
						continue;

					if (abs(axisArray[i]) > deadzone) {
						if (controllerButtonState[axis] != true) {
							lastAxisSign[i] = sign(axisArray[i]);

							controllerButtonState[axis] = true;
							WindowFrame.Parent->HandleInput(SpecialKeys[j].boundkey, KE_PRESS, false);
						}
						else {
							if (lastAxisSign[i] != sign(axisArray[i])) {
								WindowFrame.Parent->HandleInput(SpecialKeys[j].boundkey, KE_RELEASE, false);
								WindowFrame.Parent->HandleInput(SpecialKeys[j].boundkey, KE_PRESS, false);
								lastAxisSign[i] = sign(axisArray[i]);
							}
						}
					}
					else {
						if (controllerButtonState[axis] != false) {
							controllerButtonState[axis] = false;
							WindowFrame.Parent->HandleInput(SpecialKeys[j].boundkey, KE_RELEASE, false);
						}
					}
				}
			}
		}

	}
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
    }
    else
        glfwSetInputMode(wnd, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
}

bool GameWindow::SetupShaders()
{
    Log::Printf("Setting up shaders...");

    if (glGenVertexArrays && glBindVertexArray)
    {
		Log::Printf("System supports VAOs...\n");
        glGenVertexArrays(1, &defaultVao);
        glBindVertexArray(defaultVao);
    }

	Renderer::DefaultShader::Compile();
	Renderer::DefaultShader::UpdateProjection(projection);

    return true;
}

void GameWindow::AddVBO(VBO *V)
{
    VBOList.push_back(V);
}

void GameWindow::RemoveVBO(VBO *V)
{
    if (!VBOList.size()) return;

    for (auto i = VBOList.begin(); i != VBOList.end(); ++i)
    {
        if (*i == V)
        {
            VBOList.erase(i);
            return;
        }
    }
}

void GameWindow::AddShader(Renderer::Shader *S)
{
	ShaderList.push_back(S);
}

void GameWindow::RemoveShader(Renderer::Shader *S)
{
	for (auto i = ShaderList.begin(); i != ShaderList.end(); ++i) {
		if (*i == S) {
			ShaderList.erase(i);
			return;
		}
	}
}

void GameWindow::AddTTF(TruetypeFont* TTF)
{
    TTFList.push_back(TTF);
}

void GameWindow::RemoveTTF(TruetypeFont *TTF)
{
    for (std::vector<TruetypeFont*>::iterator i = TTFList.begin(); i != TTFList.end(); i++)
    {
        if (*i == TTF)
        {
            TTFList.erase(i);
            return;
        }
    }
}