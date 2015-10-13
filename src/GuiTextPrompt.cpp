#include "GameGlobal.h"
#include "BitmapFont.h"
#include "GuiTextPrompt.h"

using namespace GUI;

TextPrompt::TextPrompt()
{
	mPromptFont = nullptr;
	mOpen = false;
}

TextPrompt::~TextPrompt()
{
}

int TextPrompt::HandleInput(int32 key, KeyEventType code, bool isMouseInput)
{
	if (!mOpen)
		return 0;

	if (code == KE_RELEASE)
		return 1;

	if (BindingsManager::TranslateKey(key) == KT_Enter)
	{
		mOpen = false;
		return 2;
	}else if (BindingsManager::TranslateKey(key) == KT_BSPC)
	{
		if (mBufferText.length())
			mBufferText.erase(mBufferText.length()-1);
		return 1;
	}else if (BindingsManager::TranslateKey(key) == KT_Escape)
	{
		mOpen = false;
		return 1;
	}

	if (isprint((char)key) && !isMouseInput)
	{
		mBufferText += key;
	}

	return 1;
}

void TextPrompt::SetFont(BitmapFont* Font)
{
	if (Font)
		mPromptFont = Font;
}

void TextPrompt::SetPrompt(GString PromptText)
{
	mPromptText = PromptText;
}

void TextPrompt::Render()
{
	if (mOpen)
	{
		GString str = Utility::Format("%s\n\n%s_\n\nPress Enter to Confirm or Escape to Abort", mPromptText, mBufferText);
		if (mPromptFont)
			mPromptFont->Render(str, Vec2(100,200)); // todo: change position
	}
}

void TextPrompt::SetOpen(bool Open)
{
	mOpen = Open;

	if (mOpen)
		mBufferText.clear();
}

void TextPrompt::SwitchOpen()
{
	SetOpen(!mOpen);
}

GString TextPrompt::GetContents()
{
	return mBufferText;
}
