#include <cstdint>
#include <map>
#include <vector>
#include <string>
#include <memory>
#include <filesystem>
#include <SDL3/SDL.h>
#include <GL/glew.h>
#include <SDL3/SDL_opengl.h>
#include <rmath.h>
#include <glm.h>

#include "../game/Game.h"
#include "Logging.h"
#include "../structure/BindingsManager.h"

#include "../Application.h"
#include "GameWindow.h"
#include "ImageLoader.h"
#include "Transformation.h"
#include "Rendering.h"
#include "VBO.h"
#include "TruetypeFont.h"
// #include "RaindropRocketInterface.h"

#include "../structure/Configuration.h"
#include <text_and_file_util.h>

#include "Shader.h"
//#include <glm/gtc/matrix_transform.hpp>

#ifndef APIENTRY
#define APIENTRY
#endif

GameWindow WindowFrame;

std::map<int32_t, KeyType> BindingsManager::ScanFunction;
std::map<int32_t, int32_t> BindingsManager::ScanFunction7K;

constexpr int NUM_OF_USED_CONTROLLER_BUTTONS = 32;

int controllerToUse;
bool JoystickEnabled;
SDL_Joystick* activeJoystick = nullptr;

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
    { "LShift", SDLK_LSHIFT },
    { "RShift", SDLK_RSHIFT },
    { "Enter", SDLK_RETURN },
    { "LCtrl", SDLK_LCTRL },
    { "RCtrl", SDLK_RCTRL },
    { "LAlt", SDLK_LALT },
    { "RAlt", SDLK_RALT },
    { "Tab", SDLK_TAB },
    { "BSPC", SDLK_BACKSPACE },
    { "F1", SDLK_F1 },
    { "F2", SDLK_F2 },
    { "F3", SDLK_F3 },
    { "F4", SDLK_F4 },
    { "F5", SDLK_F5 },
    { "F6", SDLK_F6 },
    { "F7", SDLK_F7 },
    { "F8", SDLK_F8 },
    { "F9", SDLK_F9 },
    { "F10", SDLK_F10 },
    { "F11", SDLK_F11 },
    { "F12", SDLK_F12 },
    { "Supr", SDLK_DELETE },
    { "End", SDLK_END },
    { "Home", SDLK_HOME },
    { "Insert", SDLK_INSERT },
    { "PrintScreen", SDLK_PRINTSCREEN },
    { "PageDown", SDLK_PAGEDOWN },
    { "PageUp", SDLK_PAGEUP },
    { "Pause", SDLK_PAUSE },
    { "Escape", SDLK_ESCAPE },
    { "UpArrow", SDLK_UP },
    { "DownArrow", SDLK_DOWN },
    { "LeftArrow", SDLK_LEFT },
    { "RightArrow", SDLK_RIGHT },
    { "Space", SDLK_SPACE },
    { "Enter", SDLK_RETURN },
    { "Backspace", SDLK_BACKSPACE }
};

static void GLCHECKERR() {
    int err = glGetError();
    if (err) {
        Log::LogPrintf("OpenGL GamwWindow Error: %d\n", err);
    }
}

void APIENTRY OnGlDebugMsg(
        const GLenum source,
        const GLenum type,
        const GLuint id,
        const GLenum severity,
        const GLsizei length,
        const GLchar* message,
        const void* userParam) {
    std::string msg(message, message + length);
    Log::Printf("source(%d) type(%d) id(%d) severity(%d): %s",
                source, type, id, severity, msg.c_str());
}

constexpr int NUM_OF_STATIC_SPECIAL_KEYS = sizeof(StaticSpecialKeys) / sizeof(KeyAssociation); //make sure to match the above array

std::vector<KeyAssociation> SpecialKeys;

int KeyTranslate(std::string K)
{
    for (auto & SpecialKey : SpecialKeys)
    {
        std::string Key = K; otoworm::util::to_lower(Key);
        std::string Target = std::string(SpecialKey.KeyString);  otoworm::util::to_lower(Target);
        if (Key == Target)
            return SpecialKey.boundkey;
    }

    if (K.length())
    {
        if (otoworm::util::is_numeric(K.c_str()))
            return atoi(K.c_str());
        else
            return K[0];
    }
    else
        return 0;
}

struct defaultKeys_s
{
    int key;
    KeyType command;
} defaultKeys[] = {
    { SDLK_ESCAPE, KT_Escape },
    { SDLK_UP, KT_Up },
    { SDLK_DOWN, KT_Down },
    { SDLK_LEFT, KT_Left },
    { SDLK_RIGHT, KT_Right },
    { SDLK_SPACE, KT_Select },
    { SDLK_RETURN, KT_Enter },
    { SDLK_BACKSPACE, KT_BSPC },
    { SDL_BUTTON_LEFT, KT_Select },
    { SDL_BUTTON_RIGHT, KT_SelectRight },
	{ SDLK_F5, KT_ReloadScreenScripts },
	{ SDLK_F10, KT_ReloadCFG }
};

constexpr int DEFAULT_KEYS_COUNT = sizeof(defaultKeys) / sizeof(defaultKeys_s);

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
        std::string lowkey = std::string(key); otoworm::util::to_lower(lowkey);
        std::string lowname = std::string(KeytypeNames[i]); otoworm::util::to_lower(lowname);
        if (lowkey == lowname)
            return i;
    }

    return -1;
}

std::string getNameForKeytype(const KeyType K)
{
    if (K < sizeof KeytypeNames / sizeof(char*))
        return KeytypeNames[K];
    else
        return std::to_string(K);
}

std::string getNameForUntranslatedKey(const int K)
{
    for (int i = 0; i < NUM_OF_STATIC_SPECIAL_KEYS; i++)
    {
        if (StaticSpecialKeys[i].boundkey == K)
            return StaticSpecialKeys[i].KeyString;
    }

    return std::to_string(K);
}

void BindingsManager::Initialize()
{
    SpecialKeys.clear();
    for (int i = 0; i < NUM_OF_STATIC_SPECIAL_KEYS; i++)
        SpecialKeys.push_back(StaticSpecialKeys[i]);

    //controllerToUse = 1; should use this if the user entered garbage data (anything that isn't a number)
    controllerToUse = (int)Configuration::GetConfigf("ControllerNumber") - 1;

    int joystickCount = 0;
    SDL_JoystickID* joysticks = SDL_GetJoysticks(&joystickCount);
    if (joysticks && controllerToUse >= 0 && controllerToUse < joystickCount)
    {
        activeJoystick = SDL_OpenJoystick(joysticks[controllerToUse]);
        int numOfButtons = activeJoystick ? SDL_GetNumJoystickButtons(activeJoystick) : 0;
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

		int numOfAxis = activeJoystick ? SDL_GetNumJoystickAxes(activeJoystick) : 0;
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
    if (joysticks)
        SDL_free(joysticks);

    JoystickEnabled = activeJoystick != nullptr;

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
                charOut = std::string(1, static_cast<char>(defaultKeys[i].key));
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

KeyType BindingsManager::TranslateKey(const int32_t Scan)
{
    if (ScanFunction.find(Scan) != ScanFunction.end())
    {
        return ScanFunction[Scan];
    }

    return KT_Unknown;
}

int32_t BindingsManager::TranslateKey7K(const int32_t Scan)
{
    if (ScanFunction7K.find(Scan) != ScanFunction7K.end())
    {
        return ScanFunction7K[Scan];
    }

    return -1;
}


bool doFlush = false;
bool VSync = false;

GameWindow::GameWindow()
{
    Viewport.x = Viewport.y = 0;
    SizeRatio = 1.0f;
    FullscreenSwitchbackPending = false;
    CloseRequested = false;
    wnd = NULL;
    glContext = nullptr;
}

void ResizeFunc(const int32_t width, const int32_t height)
{
    float HeightRatio = (float)height / WindowFrame.GetMatrixSize().y;

	if (!WindowFrame.IsFullscreen) { // well then, let's enforce some aspect ratio
		double mwidth = WindowFrame.GetMatrixSize().x * HeightRatio;
		glViewport(0, 0, mwidth, height);
		SDL_SetWindowSize(WindowFrame.wnd, mwidth, height);

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

void InputFunc(const int32_t key, const bool pressed, const SDL_Keymod modk)
{
    WindowFrame.Parent->HandleInput(key, pressed, false);

    if (key == SDLK_RETURN && pressed && (modk & SDL_KMOD_ALT))
        WindowFrame.FullscreenSwitchbackPending = true;
}

void MouseInputFunc(const int32_t key, const bool pressed)
{
    WindowFrame.Parent->HandleInput(key, pressed, true);
}

void ScrollFunc(const double xOff, const double yOff)
{
    WindowFrame.Parent->HandleScrollInput(xOff, yOff);
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
    float mousex, mousey;
    SDL_GetMouseState(&mousex, &mousey);
    float outx = (mousex - Viewport.x) / SizeRatio;
    float outy = matrixSize.y * mousey / size.y;
    return Vec2(outx, outy);
}

Vec2 GameWindow::GetWindowMPos()
{
    float mousex, mousey;
    SDL_GetMouseState(&mousex, &mousey);
    return Vec2(mousex, mousey);
}

float GameWindow::GetWindowVScale()
{
    return SizeRatio;
}

bool GameWindow::SetupWindow()
{
    SDL_GL_MakeCurrent(wnd, glContext);

    // we have an opengl context, try opening up glew
    glewExperimental = true;
    if (const GLenum err = glewInit(); err != GLEW_OK && err != GLEW_ERROR_NO_GLX_DISPLAY)
    {
        Log::Logf("glew failed initialization: %s", glewGetErrorString(err));
        return false;
    }

    BindingsManager::Initialize();

    glEnable(GL_BLEND); GLCHECKERR();
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); GLCHECKERR();
    // glEnable(GL_CULL_FACE);
    glEnable(GL_LINE_SMOOTH); GLCHECKERR();
    // glCullFace(GL_BACK);
    // glFrontFace(GL_CW);
    // glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glEnable(GL_DEPTH_TEST); GLCHECKERR();
    glDepthFunc(GL_LEQUAL); GLCHECKERR();

	//glRenderbufferStorage(GL_RENDERBUFFER, )
	glEnable(GL_FRAMEBUFFER_SRGB); GLCHECKERR();

    // glEnable(GL_ALPHA_TEST); GLCHECKERR();
	// glEnable(GL_POLYGON_SMOOTH);
    // glAlphaFunc(GL_GREATER, 0); GLCHECKERR();

    if (VSync)
        SDL_GL_SetSwapInterval(1);

    projection = glm::ortho<float>(0.0, matrixSize.x, matrixSize.y, 0.0, -32.0, 1.0);
    projectionInverse = glm::inverse(projection);

    if (!SetupShaders())
        return false;

    GLCHECKERR();

    renderer::initialize();

    GLCHECKERR();

    if (glDebugMessageCallback) {
        // glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_MEDIUM, 0, nullptr, GL_TRUE);
        glDebugMessageCallback(OnGlDebugMsg, nullptr);
    }

    ResizeFunc(size.x, size.y);

    assert(glGetError() == 0);
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

bool GameWindow::AutoSetupWindow(Application* _parent)
{
    Parent = _parent;

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK))
    {
        Log::LogPrintf("Failure to initialize SDL: %s\n", SDL_GetError());
        return false;
	}
	else {
		Log::LogPrintf("SDL succesfully initialized.\n");
	}

	SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#ifndef NDEBUG
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif

    AssignSize();
    matrixSize.x = ScreenWidth;
    matrixSize.y = ScreenHeight;

    IsFullscreen = Configuration::GetConfigf("Fullscreen") != 0;

    doFlush = Configuration::GetConfigf("VideoFlush") != 0;
    VSync = Configuration::GetConfigf("VSync") != 0;

	if (IsFullscreen) {
		if (!SDL_GetPrimaryDisplay()) {
			Log::LogPrintf("Can't get primary display (Fullscreen)\n");
			IsFullscreen = false;
		}
	}

    SDL_WindowFlags flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
    if (IsFullscreen)
        flags |= SDL_WINDOW_FULLSCREEN;

    if (!(wnd = SDL_CreateWindow(RAINDROP_WINDOWTITLE RAINDROP_VERSIONTEXT, size.x, size.y, flags)))
    {
        Log::Logf("Failure to initialize window: %s\n", SDL_GetError());
        return false;
    }

    glContext = SDL_GL_CreateContext(wnd);
    if (!glContext)
    {
        Log::Logf("Failure to initialize OpenGL context: %s\n", SDL_GetError());
        return false;
    }

#ifdef DARWIN
    // This is a temporary hack for OS X where our size isn't getting initialized to the correct values.
    int outx = 0;
    int outy = 0;
    SDL_GetWindowSize(wnd, &outx, &outy);
    ResizeFunc(outx, outy);
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
        SDL_DisplayID display = SDL_GetPrimaryDisplay();

		if (display) {
			const SDL_DisplayMode *mode = SDL_GetCurrentDisplayMode(display);

			size.x = mode->w;
			size.y = mode->h;
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

	SDL_GL_SwapWindow(wnd);
	

    /* Fullscreen switching */

}

void GameWindow::ClearWindow()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void GameWindow::Cleanup()
{
    if (activeJoystick)
    {
        SDL_CloseJoystick(activeJoystick);
        activeJoystick = nullptr;
    }
    if (glContext)
    {
        SDL_GL_DestroyContext(glContext);
        glContext = nullptr;
    }
	SDL_DestroyWindow(wnd);
	SDL_Quit();
}

void GameWindow::UpdateFullscreen()
{
	if (FullscreenSwitchbackPending)
	{
		Log::LogPrintf("Attempting to switch fullscreen mode.\n");
        IsFullscreen = !IsFullscreen;
        if (!SDL_SetWindowFullscreen(wnd, IsFullscreen))
        {
            Log::LogPrintf("Can't switch fullscreen mode: %s\n", SDL_GetError());
            IsFullscreen = !IsFullscreen;
            FullscreenSwitchbackPending = false;
            return;
        }

        int outx = 0;
        int outy = 0;
        SDL_GetWindowSize(wnd, &outx, &outy);
        ResizeFunc(outx, outy);

		// Reload all images.
		// todo: rmlui
		// Engine::RocketInterface::ReloadTextures();
		ImageLoader::ReloadAll();

		/* This revalidates all VBOs and fonts */
		for (auto & i : VBOList)
		{
			i->invalidate();
			i->validate();
		}

		// Automatically revalidated on usage
		for (auto & i : TTFList)
		{
			i->invalidate();
		}

		FullscreenSwitchbackPending = false;
	}
}

void GameWindow::RunInput()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_EVENT_QUIT:
            CloseRequested = true;
            break;
        case SDL_EVENT_WINDOW_RESIZED:
            ResizeFunc(event.window.data1, event.window.data2);
            break;
        case SDL_EVENT_KEY_DOWN:
            if (!event.key.repeat)
                InputFunc(event.key.key, true, event.key.mod);
            break;
        case SDL_EVENT_KEY_UP:
            InputFunc(event.key.key, false, event.key.mod);
            break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            MouseInputFunc(event.button.button, true);
            break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            MouseInputFunc(event.button.button, false);
            break;
        case SDL_EVENT_MOUSE_WHEEL:
            ScrollFunc(event.wheel.x, event.wheel.y);
            break;
        case SDL_EVENT_TEXT_INPUT:
            if (event.text.text && event.text.text[0])
                WindowFrame.Parent->HandleTextInput(static_cast<unsigned int>(event.text.text[0]));
            break;
        default:
            break;
        }
    }

	if (JoystickEnabled)
	{
		// buttons
		int buttonArraySize = SDL_GetNumJoystickButtons(activeJoystick);
		if (buttonArraySize > 0)
		{
			for (int i = 0; i < buttonArraySize; i++)
			{
				for (auto & SpecialKey : SpecialKeys)
				{
					/* Matches the pressed button to its entry in the SpecialKeys vector. */
					int thisKeyNumber = SpecialKey.boundkey - 1000;
					if (i + 1 == thisKeyNumber)
					{
						/* Only processes the button push/release if the state has changed. */
                        const auto pressed = SDL_GetJoystickButton(activeJoystick, i);
						if (pressed != controllerButtonState[thisKeyNumber])
						{
							WindowFrame.Parent->HandleInput(SpecialKey.boundkey, pressed, false);
							controllerButtonState[thisKeyNumber] = pressed;
						}
					}
				}
			}
		}

		// axis
		int axisArraySize = SDL_GetNumJoystickAxes(activeJoystick);
		float deadzone = 0.25;
		if (axisArraySize) {
			for (auto i = 0; i < axisArraySize; i++) {
                const float axisValue = SDL_GetJoystickAxis(activeJoystick, i) / 32767.0f;
				for (auto & SpecialKey : SpecialKeys) {
					// as before, specialkeys vector value
					int axis = SpecialKey.boundkey - 1000;

					if ((i + buttonArraySize + 1) != axis)
						continue;

					if (abs(axisValue) > deadzone) {
						if (!controllerButtonState[axis]) {
							lastAxisSign[i] = sign(axisValue);

							controllerButtonState[axis] = true;
							WindowFrame.Parent->HandleInput(SpecialKey.boundkey, true, false);
						}
						else {
							if (lastAxisSign[i] != sign(axisValue)) {
								WindowFrame.Parent->HandleInput(SpecialKey.boundkey, false, false);
								WindowFrame.Parent->HandleInput(SpecialKey.boundkey, true, false);
								lastAxisSign[i] = sign(axisValue);
							}
						}
					}
					else {
						if (controllerButtonState[axis]) {
							controllerButtonState[axis] = false;
							WindowFrame.Parent->HandleInput(SpecialKey.boundkey, false, false);
						}
					}
				}
			}
		}

	}
}

bool GameWindow::ShouldCloseWindow()
{
    return CloseRequested;
}

void GameWindow::SetVisibleCursor(const bool Visible)
{
    if (Visible)
    {
        SDL_ShowCursor();
    }
    else
        SDL_HideCursor();
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

	renderer::DefaultShader::compile();
	renderer::DefaultShader::update_projection(projection);

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

void GameWindow::AddShader(renderer::Shader *S)
{
	ShaderList.push_back(S);
}

void GameWindow::RemoveShader(renderer::Shader *S)
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
    for (auto i = TTFList.begin(); i != TTFList.end(); ++i)
    {
        if (*i == TTF)
        {
            TTFList.erase(i);
            return;
        }
    }
}

double GameWindow::GetCurrentTime() {
    return SDL_GetTicks() / 1000.0;
}
