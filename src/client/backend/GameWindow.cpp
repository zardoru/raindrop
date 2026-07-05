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

GameWindow window;

std::map<int32_t, KeyType> BindingsManager::ScanFunction;
std::map<int32_t, int32_t> BindingsManager::ScanFunction7K;

constexpr int NUM_OF_USED_CONTROLLER_BUTTONS = 32;

int controller_to_use;
bool joystick_enabled;
SDL_Joystick* active_joystick = nullptr;

// az: wait - this is kind of a bad idea (limited size array)
// TODO: there's ought to be a better way to do this

//True is pressed, false is released
bool controller_button_state[NUM_OF_USED_CONTROLLER_BUTTONS + 1] = { 0 };
float last_axis_sign[NUM_OF_USED_CONTROLLER_BUTTONS + 1] = { 0 };
float last_axis_value[NUM_OF_USED_CONTROLLER_BUTTONS + 1] = { 0 };
//The first member of this array should never be accessed; it's there to make reading some of the code easier.

struct KeyAssociation
{
    char key_string[32];
    int bound_key;
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

void APIENTRY on_gl_debug_msg(
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

int key_translate(std::string key)
{
    for (auto & SpecialKey : SpecialKeys)
    {
        std::string key = key; otoworm::util::to_lower(key);
        auto target = std::string(SpecialKey.key_string);
        otoworm::util::to_lower(target);
        if (key == target)
            return SpecialKey.bound_key;
    }

    if (!key.empty())
    {
        if (otoworm::util::is_numeric(key.c_str()))
            return atoi(key.c_str());
        else
            return key[0];
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
const char* keytype_names[] = {
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

int get_index_for_keytype(const char* key)
{
    for (int i = 0; i < sizeof keytype_names / sizeof(char*); i++)
    {
        auto lowkey = std::string(key);
        otoworm::util::to_lower(lowkey);
        auto lowname = std::string(keytype_names[i]);
        otoworm::util::to_lower(lowname);
        if (lowkey == lowname)
            return i;
    }

    return -1;
}

std::string get_name_for_keytype(const KeyType K)
{
    if (K < sizeof keytype_names / sizeof(char*))
        return keytype_names[K];
    else
        return std::to_string(K);
}

std::string get_name_for_untranslated_key(const int K)
{
    for (auto &[key_string, bound_key] : StaticSpecialKeys)
    {
        if (bound_key == K)
            return key_string;
    }

    return std::to_string(K);
}

void BindingsManager::initialize()
{
    SpecialKeys.clear();
    for (const auto & static_special_key : StaticSpecialKeys)
        SpecialKeys.push_back(static_special_key);

    //controllerToUse = 1; should use this if the user entered garbage data (anything that isn't a number)
    controller_to_use = (int)Configuration::GetConfigf("ControllerNumber") - 1;

    int joystickCount = 0;
    SDL_JoystickID* joysticks = SDL_GetJoysticks(&joystickCount);
    if (joysticks && controller_to_use >= 0 && controller_to_use < joystickCount)
    {
        active_joystick = SDL_OpenJoystick(joysticks[controller_to_use]);
        const int num_of_buttons = active_joystick ? SDL_GetNumJoystickButtons(active_joystick) : 0;
        if (num_of_buttons)
        {
            for (int i = 1; i <= num_of_buttons; i++)
            {
                char name[32];
                sprintf(name, "Controller%d", i);
                KeyAssociation thisButton;
                strcpy(thisButton.key_string, name);
                thisButton.bound_key = 1000 + i;
                SpecialKeys.push_back(thisButton);
            }
		}

        if (const int num_of_axis = active_joystick ? SDL_GetNumJoystickAxes(active_joystick) : 0)
		{
			for (int i = num_of_buttons + 1; i <= num_of_buttons + num_of_axis; i++) {
				char name[32];
				sprintf(name, "Controller%d", i);
				KeyAssociation this_axis{};
				strcpy(this_axis.key_string, name);
				this_axis.bound_key = 1000 + i;
				SpecialKeys.push_back(this_axis);
			}
		}
    }
    if (joysticks)
        SDL_free(joysticks);

    joystick_enabled = active_joystick != nullptr;

    std::map <std::string, std::string> fields;
    Configuration::GetConfigListS("SystemKeys", fields, "");

    // key = function
    // e.g. Z = gameclick, X = gameclick
    for (const auto &[key, keytype] : fields)
    {
        // transform special name into keytype index

        // ah it's valid
        if (int idx = get_index_for_keytype(keytype.c_str()); idx != -1)
        {
            // get the key in either int or name or char format and save that into the key -> command translator

            if (int scan = key_translate(key)) // a valid key, probably
                ScanFunction[scan] = (KeyType)idx;
        }
    }

    // fill missing default keys after it's done
    for (auto &[key, keytype] : defaultKeys)
    {
        if (!ScanFunction.contains(key))
        {
            // fill the key -> command translation
            ScanFunction[key] = keytype;

            // write it out to the config file
            std::string char_out;
            if (key <= 255 && isgraph(key)) // we're not setting like, gibberish
            {
                char_out = std::string(1, static_cast<char>(key));
            }
            else
            {
                char_out = get_name_for_untranslated_key(key);
            }

            Configuration::SetConfig(char_out, get_name_for_keytype(keytype), "SystemKeys");
        }
    }

    int i = 1;
    std::map<std::string, std::string> keys;
    Configuration::GetConfigListS("Keys7K", keys, "");

    for (auto [key, val] : keys)
    {
        if (int binding = key_translate(key))
            ScanFunction7K[binding] = floor(latof(val));
    }
}

KeyType BindingsManager::translate_key(const int32_t scan)
{
    if (ScanFunction.contains(scan))
    {
        return ScanFunction[scan];
    }

    return KT_Unknown;
}

int32_t BindingsManager::translate_key_game(const int32_t scan)
{
    if (ScanFunction7K.contains(scan))
    {
        return ScanFunction7K[scan];
    }

    return -1;
}


bool do_flush = false;
bool v_sync = false;

GameWindow::GameWindow()
{
    viewport_.x = viewport_.y = 0;
    size_ratio_ = 1.0f;
    FullscreenSwitchbackPending = false;
    CloseRequested = false;
    wnd_ = NULL;
    gl_context_ = nullptr;
}

void resize_func(const int32_t width, const int32_t height)
{
    float HeightRatio = (float)height / window.get_matrix_size().y;

	if (!window.IsFullscreen) { // well then, let's enforce some aspect ratio
		double mwidth = window.get_matrix_size().x * HeightRatio;
		glViewport(0, 0, mwidth, height);
		SDL_SetWindowSize(window.wnd_, mwidth, height);

		window.size_.x = mwidth;
		window.size_.y = height;
	}
	else { // just assume the values are correct in fullscreen
		glViewport(0, 0, width, height);
		window.size_.x = width;
		window.size_.y = height;
	}

    window.size_ratio_ = HeightRatio;
}

void input_func(const int32_t key, const bool pressed, const SDL_Keymod modk)
{
    window.application_->on_input(key, pressed, false);

    if (key == SDLK_RETURN && pressed && (modk & SDL_KMOD_ALT))
        window.FullscreenSwitchbackPending = true;
}

void mouse_input_func(const int32_t key, const bool pressed)
{
    window.application_->on_input(key, pressed, true);
}

void scroll_func(const double xOff, const double yOff)
{
    window.application_->on_scroll_input(xOff, yOff);
}

Vec2 GameWindow::get_window_size() const
{
    return size_;
}

Vec2 GameWindow::get_matrix_size() const
{
    return matrix_size_;
}

Vec2 GameWindow::get_relative_mouse_pos()
{
    float mousex, mousey;
    SDL_GetMouseState(&mousex, &mousey);
    const float outx = (mousex - viewport_.x) / size_ratio_;
    const float outy = matrix_size_.y * mousey / size_.y;
    return {outx, outy};
}

Vec2 GameWindow::get_window_mouse_pos()
{
    float mousex, mousey;
    SDL_GetMouseState(&mousex, &mousey);
    return {mousex, mousey};
}

float GameWindow::get_window_v_scale() const {
    return size_ratio_;
}

bool GameWindow::setup_window()
{
    SDL_GL_MakeCurrent(wnd_, gl_context_);

    // we have an opengl context, try opening up glew
    glewExperimental = true;
    if (const GLenum err = glewInit(); err != GLEW_OK && err != GLEW_ERROR_NO_GLX_DISPLAY)
    {
        Log::LogPrintf("glew failed initialization: %s", glewGetErrorString(err));
        return false;
    }

    BindingsManager::initialize();

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

    if (v_sync)
        SDL_GL_SetSwapInterval(1);

    projection_ = glm::ortho<float>(0.0, matrix_size_.x, matrix_size_.y, 0.0, -32.0, 1.0);
    projection_inverse_ = glm::inverse(projection_);

    if (!setup_shaders())
        return false;

    GLCHECKERR();

    renderer::initialize();

    GLCHECKERR();

    if (glDebugMessageCallback) {
        // glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_MEDIUM, 0, nullptr, GL_TRUE);
        glDebugMessageCallback(on_gl_debug_msg, nullptr);
    }

    resize_func(size_.x, size_.y);

    assert(glGetError() == 0);
    return true;
}

Mat4 GameWindow::get_matrix_projection() const {
    return projection_;
}

Mat4 GameWindow::get_matrix_projection_inverse() const {
    return projection_inverse_;
}

bool GameWindow::setup(Application* _parent)
{
    application_ = _parent;

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

    assign_size();
    matrix_size_.x = ScreenWidth;
    matrix_size_.y = ScreenHeight;

    IsFullscreen = Configuration::GetConfigf("Fullscreen") != 0;

    do_flush = Configuration::GetConfigf("VideoFlush") != 0;
    v_sync = Configuration::GetConfigf("VSync") != 0;

	if (IsFullscreen) {
		if (!SDL_GetPrimaryDisplay()) {
			Log::LogPrintf("Can't get primary display (Fullscreen)\n");
			IsFullscreen = false;
		}
	}

    SDL_WindowFlags flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
    if (IsFullscreen)
        flags |= SDL_WINDOW_FULLSCREEN;

    if (!((wnd_ = SDL_CreateWindow(RAINDROP_WINDOWTITLE RAINDROP_VERSIONTEXT, size_.x, size_.y, flags))))
    {
        Log::Logf("Failure to initialize window: %s\n", SDL_GetError());
        return false;
    }

    gl_context_ = SDL_GL_CreateContext(wnd_);
    if (!gl_context_)
    {
        Log::Logf("Failure to initialize OpenGL context: %s\n", SDL_GetError());
        return false;
    }

#ifdef DARWIN
    // This is a temporary hack for OS X where our size isn't getting initialized to the correct values.
    int outx = 0;
    int outy = 0;
    SDL_GetWindowSize(wnd_, &outx, &outy);
    resize_func(outx, outy);
#endif

    set_visible_cursor(Configuration::GetSkinConfigf("ShowCursor") != 0);

    return setup_window();
}

void GameWindow::assign_size()
{
    float window_width = Configuration::GetConfigf("WindowWidth");
    float window_height = Configuration::GetConfigf("WindowHeight");

    if (window_width == 0 || window_height == 0)
    {
        SDL_DisplayID display = SDL_GetPrimaryDisplay();

		if (display) {
			const SDL_DisplayMode *mode = SDL_GetCurrentDisplayMode(display);

			size_.x = mode->w;
			size_.y = mode->h;
		}
		else {
			Log::LogPrintf("Monitor == null? Defaulting to 1024x768.");
			window_width = 1024;
			window_height = 768;
			goto autosize;
		}
    }
    else
    {
		autosize:
        size_.x = window_width;
        size_.y = window_height;
    }
}

void GameWindow::swap_buffers() const {
	if (do_flush)
		glFlush();

	SDL_GL_SwapWindow(wnd_);
	

    /* Fullscreen switching */

}

void GameWindow::clear_window()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void GameWindow::cleanup()
{
    if (active_joystick)
    {
        SDL_CloseJoystick(active_joystick);
        active_joystick = nullptr;
    }
    if (gl_context_)
    {
        SDL_GL_DestroyContext(gl_context_);
        gl_context_ = nullptr;
    }
	SDL_DestroyWindow(wnd_);
	SDL_Quit();
}

void GameWindow::update_fullscreen()
{
	if (FullscreenSwitchbackPending)
	{
		Log::LogPrintf("Attempting to switch fullscreen mode.\n");
        IsFullscreen = !IsFullscreen;
        if (!SDL_SetWindowFullscreen(wnd_, IsFullscreen))
        {
            Log::LogPrintf("Can't switch fullscreen mode: %s\n", SDL_GetError());
            IsFullscreen = !IsFullscreen;
            FullscreenSwitchbackPending = false;
            return;
        }

        int outx = 0;
        int outy = 0;
        SDL_GetWindowSize(wnd_, &outx, &outy);
        resize_func(outx, outy);

		// Reload all images.
		// todo: rmlui
		// Engine::RocketInterface::ReloadTextures();
		ImageLoader::ReloadAll();

		/* This revalidates all VBOs and fonts */
		for (const auto & i : vbo_list_)
		{
			i->invalidate();
			i->validate();
		}

		// Automatically revalidated on usage
		for (auto & i : ttf_list_)
		{
			i->invalidate();
		}

		FullscreenSwitchbackPending = false;
	}
}

void GameWindow::run_input()
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
            resize_func(event.window.data1, event.window.data2);
            break;
        case SDL_EVENT_KEY_DOWN:
            if (!event.key.repeat)
                input_func(event.key.key, true, event.key.mod);
            break;
        case SDL_EVENT_KEY_UP:
            input_func(event.key.key, false, event.key.mod);
            break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            mouse_input_func(event.button.button, true);
            break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            mouse_input_func(event.button.button, false);
            break;
        case SDL_EVENT_MOUSE_WHEEL:
            scroll_func(event.wheel.x, event.wheel.y);
            break;
        case SDL_EVENT_TEXT_INPUT:
            if (event.text.text && event.text.text[0])
                window.application_->on_text_input(static_cast<unsigned int>(event.text.text[0]));
            break;
        default:
            break;
        }
    }

	if (joystick_enabled)
	{
		// buttons
		const int button_array_size = SDL_GetNumJoystickButtons(active_joystick);
		if (button_array_size > 0)
		{
			for (int i = 0; i < button_array_size; i++)
			{
				for (const auto &[key_string, bound_key] : SpecialKeys)
				{
					/* Matches the pressed button to its entry in the SpecialKeys vector. */
                    if (const int this_key_number = bound_key - 1000; i + 1 == this_key_number)
					{
						/* Only processes the button push/release if the state has changed. */
                        if (const auto pressed = SDL_GetJoystickButton(active_joystick, i);
                            pressed != controller_button_state[this_key_number])
						{
							window.application_->on_input(bound_key, pressed, false);
							controller_button_state[this_key_number] = pressed;
						}
					}
				}
			}
		}

		// axis
        if (const int axis_array_size = SDL_GetNumJoystickAxes(active_joystick)) {
			for (auto i = 0; i < axis_array_size; i++) {
                const float axis_value = SDL_GetJoystickAxis(active_joystick, i) / 32767.0f;
				for (auto &[key_string, bound_key] : SpecialKeys) {
					// as before, specialkeys vector value
					const int axis = bound_key - 1000;

					if ((i + button_array_size + 1) != axis)
						continue;

					if (constexpr float deadzone = 0.25; abs(axis_value) > deadzone) {
						if (!controller_button_state[axis]) {
							last_axis_sign[i] = sign(axis_value);

							controller_button_state[axis] = true;
							window.application_->on_input(bound_key, true, false);
						}
						else {
							if (last_axis_sign[i] != sign(axis_value)) {
								window.application_->on_input(bound_key, false, false);
								window.application_->on_input(bound_key, true, false);
								last_axis_sign[i] = sign(axis_value);
							}
						}
					}
					else {
						if (controller_button_state[axis]) {
							controller_button_state[axis] = false;
							window.application_->on_input(bound_key, false, false);
						}
					}
				}
			}
		}

	}
}

bool GameWindow::should_close_window() const {
    return CloseRequested;
}

void GameWindow::set_visible_cursor(const bool visible)
{
    if (visible)
    {
        SDL_ShowCursor();
    }
    else
        SDL_HideCursor();
}

bool GameWindow::setup_shaders()
{
    Log::Printf("Setting up shaders...\n");

    if (glGenVertexArrays && glBindVertexArray)
    {
		Log::Printf("System supports VAOs...\n");
        glGenVertexArrays(1, &default_vao_);
        glBindVertexArray(default_vao_);
    }

	renderer::DefaultShader::compile();
	renderer::DefaultShader::update_projection(projection_);

    return true;
}

void GameWindow::add_vbo(VBO *v)
{
    vbo_list_.push_back(v);
}

void GameWindow::remove_vbo(VBO *v)
{
    if (!vbo_list_.size()) return;

    for (auto i = vbo_list_.begin(); i != vbo_list_.end(); ++i)
    {
        if (*i == v)
        {
            vbo_list_.erase(i);
            return;
        }
    }
}

void GameWindow::add_shader(renderer::Shader *s)
{
	shader_list_.push_back(s);
}

void GameWindow::remove_shader(renderer::Shader *s)
{
	for (auto i = shader_list_.begin(); i != shader_list_.end(); ++i) {
		if (*i == s) {
			shader_list_.erase(i);
			return;
		}
	}
}

void GameWindow::add_ttf(TruetypeFont* ttf)
{
    ttf_list_.push_back(ttf);
}

void GameWindow::remove_ttf(TruetypeFont *ttf)
{
    for (auto i = ttf_list_.begin(); i != ttf_list_.end(); ++i)
    {
        if (*i == ttf)
        {
            ttf_list_.erase(i);
            return;
        }
    }
}

double GameWindow::get_current_time() {
    return SDL_GetTicks() / 1000.0;
}
