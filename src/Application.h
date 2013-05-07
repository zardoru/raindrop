
class Application 
{
	double oldTime;
	IScreen *Game;

public:

	Application();

	void HandleInput(int32 key, int32 state, bool isMouseInput);

	void Init();
	void Run();
	void Close();
};

extern Application App;