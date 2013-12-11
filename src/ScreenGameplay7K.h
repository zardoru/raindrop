#ifndef SG7K_H_
#define SG7K_H_

class ScreenGameplay7K : public IScreen
{
private:
	int Measure;
	float Speed, SpeedMultiplier;
	float SongOldTime;
	glm::mat4 PositionMatrix;
	Song7K *MySong;

	PaStreamWrapper *Music;

	GraphObject2D Keys[16];

public:
	ScreenGameplay7K();
	void Init(Song7K *S);
	void LoadThreadInitialization();
	void MainThreadInitialization();
	void Cleanup();

	bool Run(double Delta);
	void HandleInput(int32 key, KeyEventType code, bool isMouseInput);
};

#endif