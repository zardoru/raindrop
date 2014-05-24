#include "GameGlobal.h"
#include "Image.h"
#include "GraphObject2D.h"
#include "BitmapFont.h"
#include "GuiTextPrompt.h"

using namespace GUI;

TextPrompt::TextPrompt()
{
	mPromptFont = NULL;
	mOpen = false;
}

TextPrompt::~TextPrompt()
{
}

int TextPrompt::HandleInput(int32 key, KeyEventType code, bool isMouseInput)
{
	if (!mOpen)
		return 0;

	if (code == KE_Release)
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

void TextPrompt::SetPrompt(String PromptText)
{
	mPromptText = PromptText;
}

void TextPrompt::Render()
{
	if (mOpen)
	{
		std::stringstream ss;
		ss << mPromptText << "\n\n" << mBufferText << "_" << "\n\n" << "Press Enter To Confirm or Escape to Abort";
		if (mPromptFont)
			mPromptFont->DisplayText(ss.str().c_str(), Vec2(100,200)); // todo: change position
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

String TextPrompt::GetContents()
{
	return mBufferText;
}
