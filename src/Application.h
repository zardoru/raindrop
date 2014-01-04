
class Application 
{
	double oldTime;
	IScreen *Game;

public:

	Application();

	void HandleInput(int32 key, KeyEventType state, bool isMouseInput);
	void HandleScrollInput(double xOff, double yOff);

	void Init();
	void Run();
	void Close();
};

extern Application App;