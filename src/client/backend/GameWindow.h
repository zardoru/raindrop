#pragma once

#include <SDL3/SDL_video.h>
#include <SDL3/SDL_keyboard.h>

class VBO;
class Application;
class TruetypeFont;

namespace renderer {
	class Shader;
}

class GameWindow
{
    friend void resize_func(int32_t, int32_t);
    friend void input_func(int32_t, bool, SDL_Keymod);
    friend void mouse_input_func(int32_t, bool);
    friend void scroll_func(double, double);

    Vec2 size_;
    Vec2 matrix_size_, viewport_;
    Mat4 projection_;
    Mat4 projection_inverse_;

    uint32_t default_vao_;
    SDL_Window *wnd_;
    SDL_GLContext gl_context_;
    float size_ratio_;

    bool setup_window();
    bool setup_shaders();
    void assign_size();

    std::vector<VBO*> vbo_list_;
    std::vector<TruetypeFont*> ttf_list_;
	std::vector<renderer::Shader*> shader_list_;

    Application* application_;
    bool FullscreenSwitchbackPending, IsFullscreen, CloseRequested;

public:
    GameWindow();
    bool setup(Application* _app);
    void clear_window(); // basically wrapping up glClear
    void cleanup();

	void update_fullscreen();
	void run_input();

    void set_visible_cursor(bool visible);
    void add_vbo(VBO* v);
    void remove_vbo(VBO *v);

	void add_shader(renderer::Shader *s);
	void remove_shader(renderer::Shader *s);

    void add_ttf(TruetypeFont* ttf);
    void remove_ttf(TruetypeFont* ttf);

    Mat4 get_matrix_projection() const;
    Mat4 get_matrix_projection_inverse() const;

    float get_window_v_scale() const;

    // returns the mouse position relative to the matrix and window size
    Vec2 get_relative_mouse_pos();

    // returns the mouse position relative to the window.
    Vec2 get_window_mouse_pos();

    // returns the size of the window
    Vec2 get_window_size() const;

    // returns the size of the orthogonal matrix
    Vec2 get_matrix_size() const;

    /* using OpenGL -1 to 1 range*/

    bool should_close_window() const;
    void swap_buffers() const;
    double get_current_time();
};

extern GameWindow window;
