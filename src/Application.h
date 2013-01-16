
class Application 
{
	double oldTime;
	IScreen *Game;

public:

	Application();

	void HandleInput(int key, int state, bool isMouseInput);

	void Init();
	void Run();
	void Close();
};

extern Application App;