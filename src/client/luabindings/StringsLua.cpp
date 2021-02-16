#include <string>
#include <memory>
#include <glm.h>
#include <vector>
#include <map>
#include <filesystem>
#include <rmath.h>

#include "Texture.h"

#include "Transformation.h"
#include "Rendering.h"
#include "Sprite.h"

#include "Font.h"
#include "TruetypeFont.h"
#include "BitmapFont.h"
#include "GraphicalString.h"

#include "../LuaManager.h"
#include <LuaBridge/LuaBridge.h>

void LoadBmFont(BitmapFont* B, std::string Fn, float CellWidth, float CellHeight, float CharWidth, float CharHeight, int startChar)
{
	Vec2 Size(CharWidth, CharHeight);
	Vec2 CellSize(CellWidth, CellHeight);
	B->LoadSkinFontImage(Fn.c_str(), Size, CellSize, Size, startChar);
}

/// Font and string types. Instantiate a font with TruetypeFont() or LoadBitmapFont() - they are on the "Font" namespace.
/// @engineclass Strings
void CreateStringsLuaInterface(LuaManager* AnimLua)
{

	/// Base class for fonts.
	// @type Font.Font
	luabridge::getGlobalNamespace(AnimLua->GetState())
		.beginNamespace("Fonts")
		.beginClass<Font>("Font")
		/// Set font color.
		// @function SetColor
		// @param r Red, from 0 to 1.
		// @param g Green, from 0 to 1.
		// @param b Blue, from 0 to 1.
		.addFunction("SetColor", &Font::SetColor)
		/// Set font alpha
		// @function SetAlpha
		// @param alpha New alpha value.
		.addFunction("SetAlpha", &Font::SetAlpha)
		/// Get horizontal length for a string.
		// @function GetLength
		// @tparam string s The string to get the length of.
		.addFunction("GetLength", &Font::GetHorizontalLength)
		.endClass()
		/// @type Font.TruetypeFont
		.deriveClass <TruetypeFont, Font>("TruetypeFont")
		/// Constructor of a truetype font.
		// @function TruetypeFont
		// @tparam string path The path to the font, relative to the working directory of the application.
		.addConstructor <void(*) (std::string)>()
		.endClass()
		.deriveClass <BitmapFont, Font>("BitmapFont")
		.addConstructor<void(*)()>()
		.endClass()
		.addFunction("LoadBitmapFont", LoadBmFont)
		.endNamespace();

	/// The class to display strings on raindrop. Inherits from @{Object2D}
	/// @type StringObject2D
	luabridge::getGlobalNamespace(AnimLua->GetState())
		.deriveClass<GraphicalString, Sprite>("StringObject2D")
		/// Create a new instance of StringObject2D.
		// @function StringObject2D
		.addConstructor <void(*) ()>()
		/// Sets the font of this string.
		// @property Font
		.addProperty("Font", &GraphicalString::GetFont, &GraphicalString::SetFont)
		/// Sets the text of this string.
		// @property Text
		.addProperty("Text", &GraphicalString::GetText, &GraphicalString::SetText)
		/// Sets the font size for this string.
		// @property FontSize
		.addProperty("FontSize", &GraphicalString::GetFontSize, &GraphicalString::SetFontSize)
		/// Returns the length of the currently set string.
		// @property TextSize
		.addProperty("TextSize", &GraphicalString::GetTextSize)
		/// Sets the kerning scale. 
		// @property KernScale
		.addProperty("KernScale", &GraphicalString::GetKerningScale, &GraphicalString::SetKerningScale)
		// .addProperty("ChainTransformation", &O2DProxy::getChainTransformation<GraphicalString>, &O2DProxy::setChainTransformation<GraphicalString>)
		.endClass();
}