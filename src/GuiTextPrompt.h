#pragma once

class BitmapFont;

namespace GUI
{

class TextPrompt
{
private:
	BitmapFont* mPromptFont;
	std::string		mPromptText;
	std::string		mBufferText;
	bool		mOpen;
public:
	TextPrompt();
	~TextPrompt();
	/* Returns 0 if closed, 1 if handling input and 2 if there's data to get from the prompt. */
	int HandleInput(int32_t key, KeyEventType code, bool isMouseInput);
	void SetFont(BitmapFont* Font);
	void SetPrompt(std::string PromptText);
	void Render();
	void SetOpen(bool Open);
	void SwitchOpen();
	std::string GetContents();
};

}