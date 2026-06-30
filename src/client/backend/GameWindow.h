#pragma once

#include <SDL3/SDL_video.h>
#include <SDL3/SDL_keyboard.h>

class VBO;
class Application;
class TruetypeFont;

namespace Renderer {
	class Shader;
}

class GameWindow
{
    friend void ResizeFunc(int32_t, int32_t);
    friend void InputFunc(int32_t, bool, SDL_Keymod);
    friend void MouseInputFunc(int32_t, bool);
    friend void ScrollFunc(double, double);

    Vec2 size;
    Vec2 matrixSize, Viewport;
    Mat4 projection;
    Mat4 projectionInverse;

    uint32_t defaultVao;
    SDL_Window *wnd;
    SDL_GLContext glContext;
    float SizeRatio;

    bool SetupWindow();
    bool SetupShaders();
    void AssignSize();

    std::vector<VBO*> VBOList;
    std::vector<TruetypeFont*> TTFList;
	std::vector<Renderer::Shader*> ShaderList;

    Application* Parent;
    bool FullscreenSwitchbackPending, IsFullscreen, CloseRequested;

public:
    GameWindow();
    bool AutoSetupWindow(Application* Parent);
    void ClearWindow(); // basically wrapping up glClear
    void Cleanup();

	void UpdateFullscreen();
	void RunInput();

    void SetVisibleCursor(bool Visible);
    void AddVBO(VBO* V);
    void RemoveVBO(VBO *V);

	void AddShader(Renderer::Shader *S);
	void RemoveShader(Renderer::Shader *S);

    void AddTTF(TruetypeFont* TTF);
    void RemoveTTF(TruetypeFont* TTF);

    Mat4 GetMatrixProjection();
    Mat4 GetMatrixProjectionInverse();

    float GetWindowVScale();

    // returns the mouse position relative to the matrix and window size
    Vec2 GetRelativeMPos();

    // returns the mouse position relative to the window.
    Vec2 GetWindowMPos();

    // returns the size of the window
    Vec2 GetWindowSize() const;

    // returns the size of the orthogonal matrix
    Vec2 GetMatrixSize() const;

    /* using OpenGL -1 to 1 range*/

    bool ShouldCloseWindow();
    void SwapBuffers();
    double GetCurrentTime();
};

extern GameWindow WindowFrame;
