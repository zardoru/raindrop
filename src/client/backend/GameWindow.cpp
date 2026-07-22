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
#include "TextureCollection.h"
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

std::map<int32_t, KeyType> BindingsManager::ScanFunction;
std::map<int32_t, int32_t> BindingsManager::ScanFunction7K;

struct KeyAssociation
{
    char key_string[32];
    int bound_key;
    int controller = -1;
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

struct JoystickInstance {
    std::unique_ptr<SDL_Joystick, decltype(&SDL_CloseJoystick)> active_joystick;
    std::vector<bool> controller_button_state;
    std::vector<float> last_axis_sign;
    uint32_t key_start;
    SDL_JoystickID controller_id;

    JoystickInstance(JoystickInstance&& other) noexcept : active_joystick(nullptr, SDL_CloseJoystick) {
        active_joystick = std::move(other.active_joystick);
        controller_button_state = std::move(other.controller_button_state);
        last_axis_sign = std::move(other.last_axis_sign);
        key_start = other.key_start;
        controller_id = other.controller_id;
    }

    JoystickInstance(const SDL_JoystickID joystick_id, std::vector<KeyAssociation>& special_keys) :
        active_joystick(SDL_OpenJoystick(joystick_id), SDL_CloseJoystick), controller_id(joystick_id) {
        key_start = 1000 * joystick_id;

        const int num_of_buttons = active_joystick ? SDL_GetNumJoystickButtons(active_joystick.get()) : 0;
        if (num_of_buttons)
        {
            for (int i = 1; i <= num_of_buttons; i++)
            {
                char name[32];
                snprintf(name, sizeof name, "Controller%dBtn%d", joystick_id, i);
                KeyAssociation this_button{};
                strcpy(this_button.key_string, name);
                this_button.bound_key = key_start + i;
                this_button.controller = joystick_id;
                special_keys.push_back(this_button);
            }
        }

        const int num_of_axes = active_joystick ? SDL_GetNumJoystickAxes(active_joystick.get()) : 0;
        for (int i = 1; i <= num_of_axes; i++)
        {
            char name[32];
            snprintf(name, sizeof name, "Controller%dAxis%d", joystick_id, i);
            KeyAssociation this_axis{};
            strcpy(this_axis.key_string, name);
            this_axis.bound_key = key_start + i + num_of_buttons;
            this_axis.controller = joystick_id;
            special_keys.push_back(this_axis);
        }

        controller_button_state.resize(num_of_buttons + num_of_axes + 1);
        last_axis_sign.resize(num_of_axes);
    }

    ~JoystickInstance() = default;

    void run_input(const Application* application, const std::vector<KeyAssociation>& special_keys)
    {
        if (!active_joystick)
            return;

        const int button_array_size = SDL_GetNumJoystickButtons(active_joystick.get());
        for (int i = 0; i < button_array_size; i++)
        {
            for (const auto& [key_string, bound_key, controller] : special_keys)
            {
                if (controller != controller_id)
                    continue;

                const int this_key_number = bound_key - key_start;
                if (i + 1 != this_key_number)
                    continue;

                const auto pressed = SDL_GetJoystickButton(active_joystick.get(), i);
                if (pressed != controller_button_state[this_key_number])
                {
                    application->on_input(bound_key, pressed, false);
                    controller_button_state[this_key_number] = pressed;
                }
            }
        }

        const int axis_array_size = SDL_GetNumJoystickAxes(active_joystick.get());
        for (int i = 0; i < axis_array_size; i++)
        {
            const float axis_value = SDL_GetJoystickAxis(active_joystick.get(), i) / 32767.0f;
            for (const auto& [key_string, bound_key, controller] : special_keys)
            {
                if (controller != controller_id)
                    continue;

                const int axis = bound_key - key_start;
                if (i + button_array_size + 1 != axis)
                    continue;

                if (constexpr float deadzone = 0.25; abs(axis_value) > deadzone)
                {
                    if (!controller_button_state[axis])
                    {
                        last_axis_sign[i] = sign(axis_value);
                        controller_button_state[axis] = true;
                        application->on_input(bound_key, true, false);
                    }
                    else if (last_axis_sign[i] != sign(axis_value))
                    {
                        application->on_input(bound_key, false, false);
                        application->on_input(bound_key, true, false);
                        last_axis_sign[i] = sign(axis_value);
                    }
                }
                else if (controller_button_state[axis])
                {
                    controller_button_state[axis] = false;
                    application->on_input(bound_key, false, false);
                }
            }
        }
    }
};

struct JoystickSupport
{
    std::vector<JoystickInstance> controllers_;
    ~JoystickSupport() = default;

    void initialize(std::vector<KeyAssociation>& special_keys)
    {
        controllers_.clear();

        int joystick_count = 0;
        SDL_JoystickID* joysticks = SDL_GetJoysticks(&joystick_count);
        if (joysticks)
        {
            for (int i = 0; i < joystick_count; ++i)
            {
                controllers_.emplace_back(joysticks[i], special_keys);
            }
        }

        if (joysticks)
            SDL_free(joysticks);
    }

    void run_input(const Application* application, const std::vector<KeyAssociation>& special_keys)
    {
        for (auto &joy : controllers_) {
            joy.run_input(application, special_keys);
        }
    }

    void cleanup()
    {
        controllers_.clear();
    }
};

int32_t normalize_sdl_key(const int32_t key)
{
    // Keep the key values used by the existing config.ini and Lua skins.
    // They were GLFW key values before the SDL migration.
    switch (key)
    {
    case SDLK_ESCAPE: return 256;
    case SDLK_RETURN: return 257;
    case SDLK_TAB: return 258;
    case SDLK_BACKSPACE: return 259;
    case SDLK_INSERT: return 260;
    case SDLK_DELETE: return 261;
    case SDLK_RIGHT: return 262;
    case SDLK_LEFT: return 263;
    case SDLK_DOWN: return 264;
    case SDLK_UP: return 265;
    case SDLK_PAGEUP: return 266;
    case SDLK_PAGEDOWN: return 267;
    case SDLK_HOME: return 268;
    case SDLK_END: return 269;
    case SDLK_CAPSLOCK: return 280;
    case SDLK_SCROLLLOCK: return 281;
    case SDLK_NUMLOCKCLEAR: return 282;
    case SDLK_PRINTSCREEN: return 283;
    case SDLK_PAUSE: return 284;
    case SDLK_F1: return 290;
    case SDLK_F2: return 291;
    case SDLK_F3: return 292;
    case SDLK_F4: return 293;
    case SDLK_F5: return 294;
    case SDLK_F6: return 295;
    case SDLK_F7: return 296;
    case SDLK_F8: return 297;
    case SDLK_F9: return 298;
    case SDLK_F10: return 299;
    case SDLK_F11: return 300;
    case SDLK_F12: return 301;
    case SDLK_F13: return 302;
    case SDLK_F14: return 303;
    case SDLK_F15: return 304;
    case SDLK_F16: return 305;
    case SDLK_F17: return 306;
    case SDLK_F18: return 307;
    case SDLK_F19: return 308;
    case SDLK_F20: return 309;
    case SDLK_F21: return 310;
    case SDLK_F22: return 311;
    case SDLK_F23: return 312;
    case SDLK_F24: return 313;
    case SDLK_KP_0: return 320;
    case SDLK_KP_1: return 321;
    case SDLK_KP_2: return 322;
    case SDLK_KP_3: return 323;
    case SDLK_KP_4: return 324;
    case SDLK_KP_5: return 325;
    case SDLK_KP_6: return 326;
    case SDLK_KP_7: return 327;
    case SDLK_KP_8: return 328;
    case SDLK_KP_9: return 329;
    case SDLK_KP_DECIMAL: return 330;
    case SDLK_KP_DIVIDE: return 331;
    case SDLK_KP_MULTIPLY: return 332;
    case SDLK_KP_MINUS: return 333;
    case SDLK_KP_PLUS: return 334;
    case SDLK_KP_ENTER: return 335;
    case SDLK_KP_EQUALS: return 336;
    case SDLK_LSHIFT: return 340;
    case SDLK_LCTRL: return 341;
    case SDLK_LALT: return 342;
    case SDLK_LGUI: return 343;
    case SDLK_RSHIFT: return 344;
    case SDLK_RCTRL: return 345;
    case SDLK_RALT: return 346;
    case SDLK_RGUI: return 347;
    case SDLK_MENU: return 348;
    default:
        break;
    }

    // GLFW used uppercase ASCII values for alphabetic keys; SDL keycodes are
    // layout-dependent lowercase Unicode values by default.
    if (key >= 'a' && key <= 'z')
        return key - ('a' - 'A');

    return key;
}

int key_translate(const std::string &key)
{
    for (auto& [key_string, bound_key, controller] : SpecialKeys)
    {
        std::string nkey = key; otoworm::util::to_lower(nkey);
        auto target = std::string(key_string);
        otoworm::util::to_lower(target);
        if (nkey == target)
            return bound_key;
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
    for (auto& [key_string, bound_key, controller] : StaticSpecialKeys)
    {
        if (bound_key == K)
            return key_string;
    }

    return std::to_string(K);
}

void BindingsManager::initialize()
{
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
            {
                ScanFunction[scan] = (KeyType)idx;
                ScanFunction[normalize_sdl_key(scan)] = (KeyType)idx;
            }
        }
    }

    // fill missing default keys after it's done
    for (auto &[key, keytype] : defaultKeys)
    {
        if (!ScanFunction.contains(key))
        {
            // fill the key -> command translation
            ScanFunction[key] = keytype;
            ScanFunction[normalize_sdl_key(key)] = keytype;

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
        {
            ScanFunction7K[binding] = floor(latof(val));
            ScanFunction7K[normalize_sdl_key(binding)] = floor(latof(val));
        }
    }
}

KeyType BindingsManager::translate_key(const int32_t scan)
{
    if (ScanFunction.contains(scan))
    {
        return ScanFunction[scan];
    }

    const auto normalized = normalize_sdl_key(scan);
    if (ScanFunction.contains(normalized))
        return ScanFunction[normalized];

    return KT_Unknown;
}

int32_t BindingsManager::translate_key_game(const int32_t scan)
{
    if (ScanFunction7K.contains(scan))
    {
        return ScanFunction7K[scan];
    }

    const auto normalized = normalize_sdl_key(scan);
    if (ScanFunction7K.contains(normalized))
        return ScanFunction7K[normalized];

    return -1;
}


bool do_flush = false;
bool v_sync = false;

GameWindow::GameWindow()
{
    viewport_.x = viewport_.y = 0;
    size_ratio_ = 1.0f;
    fullscreen_switchback_pending_ = false;
    close_requested_ = false;
    wnd_ = nullptr;
    gl_context_ = nullptr;
    joystick_support_ = std::make_unique<JoystickSupport>();
}

GameWindow::~GameWindow() = default;

GameWindow& GameWindow::get_instance()
{
    static GameWindow instance;
    return instance;
}

void resize_func(const int32_t width, const int32_t height)
{
    auto& game_window = GameWindow::get_instance();
    float HeightRatio = (float)height / game_window.get_matrix_size().y;

	if (!game_window.is_fullscreen_) { // well then, let's enforce some aspect ratio
		double mwidth = game_window.get_matrix_size().x * HeightRatio;
		glViewport(0, 0, mwidth, height);
		SDL_SetWindowSize(game_window.wnd_, mwidth, height);

		game_window.size_.x = mwidth;
		game_window.size_.y = height;
	}
	else { // just assume the values are correct in fullscreen
		glViewport(0, 0, width, height);
		game_window.size_.x = width;
		game_window.size_.y = height;
	}

    game_window.size_ratio_ = HeightRatio;
}

void input_func(const int32_t key, const bool pressed, const SDL_Keymod modk)
{
    auto& game_window = GameWindow::get_instance();
    game_window.application_->on_input(key, pressed, false);

    if (key == normalize_sdl_key(SDLK_RETURN) && pressed && (modk & SDL_KMOD_ALT))
        game_window.fullscreen_switchback_pending_ = true;
}

void mouse_input_func(const int32_t key, const bool pressed)
{
    GameWindow::get_instance().application_->on_input(key, pressed, true);
}

void scroll_func(const double xOff, const double yOff)
{
    GameWindow::get_instance().application_->on_scroll_input(xOff, yOff);
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

    SpecialKeys.clear();
    for (const auto& static_special_key : StaticSpecialKeys)
        SpecialKeys.push_back(static_special_key);

    joystick_support_->initialize(SpecialKeys);
    BindingsManager::initialize();

    glEnable(GL_BLEND); GLCHECKERR();
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); GLCHECKERR();
    // glEnable(GL_CULL_FACE);
    glEnable(GL_LINE_SMOOTH); GLCHECKERR();
    // glCullFace(GL_BACK);
    // glFrontFace(GL_CW);
    // glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glDisable(GL_DEPTH_TEST); GLCHECKERR();

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

    is_fullscreen_ = Configuration::GetConfigf("Fullscreen") != 0;

    do_flush = Configuration::GetConfigf("VideoFlush") != 0;
    v_sync = Configuration::GetConfigf("VSync") != 0;

	if (is_fullscreen_) {
		if (!SDL_GetPrimaryDisplay()) {
			Log::LogPrintf("Can't get primary display (Fullscreen)\n");
			is_fullscreen_ = false;
		}
	}

    SDL_WindowFlags flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
    if (is_fullscreen_)
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
    joystick_support_->cleanup();
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
	if (fullscreen_switchback_pending_)
	{
		Log::LogPrintf("Attempting to switch fullscreen mode.\n");
        is_fullscreen_ = !is_fullscreen_;
        if (!SDL_SetWindowFullscreen(wnd_, is_fullscreen_))
        {
            Log::LogPrintf("Can't switch fullscreen mode: %s\n", SDL_GetError());
            is_fullscreen_ = !is_fullscreen_;
            fullscreen_switchback_pending_ = false;
            return;
        }

        int outx = 0;
        int outy = 0;
        SDL_GetWindowSize(wnd_, &outx, &outy);
        resize_func(outx, outy);

		TextureCollection::reload_all();

		/* This revalidates all VBOs and fonts */
		for (const auto & i : vbo_list_)
		{
			i->invalidate();
			i->validate();
		}

		// Automatically revalidated on usage
		for (const auto & i : ttf_list_)
		{
			i->invalidate();
		}

		fullscreen_switchback_pending_ = false;
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
            close_requested_ = true;
            break;
        case SDL_EVENT_WINDOW_RESIZED:
            resize_func(event.window.data1, event.window.data2);
            break;
        case SDL_EVENT_KEY_DOWN:
            if (!event.key.repeat)
                input_func(normalize_sdl_key(event.key.key), true, event.key.mod);
            break;
        case SDL_EVENT_KEY_UP:
            input_func(normalize_sdl_key(event.key.key), false, event.key.mod);
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
                application_->on_text_input(static_cast<unsigned int>(event.text.text[0]));
            break;
        default:
            break;
        }
    }

    joystick_support_->run_input(application_, SpecialKeys);
}

bool GameWindow::should_close_window() const {
    return close_requested_;
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

	renderer::Shader::Default::compile();
	renderer::Shader::Default::update_projection(projection_);

    return true;
}

void GameWindow::add_vbo(VBO *v)
{
    vbo_list_.push_back(v);
}

void GameWindow::remove_vbo(const VBO *v)
{
    if (vbo_list_.empty()) return;

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

void GameWindow::remove_shader(const renderer::Shader *s)
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

void GameWindow::remove_ttf(const TruetypeFont *ttf)
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
