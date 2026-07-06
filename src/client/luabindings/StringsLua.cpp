#include <string>
#include <memory>
#include <glm.h>
#include <vector>
#include <map>
#include <filesystem>
#include <rmath.h>

#include "Texture2D.h"

#include "Transformation.h"
#include "Rendering.h"
#include "Sprite.h"

#include "Font.h"
#include "TruetypeFont.h"
#include "BitmapFont.h"
#include "GraphicalString.h"

#include "LuaManager.h"
#include <LuaBridge/LuaBridge.h>

void LoadBmFont(BitmapFont* B, std::string Fn, float CellWidth, float CellHeight, float CharWidth, float CharHeight, int startChar)
{
	Vec2 Size(CharWidth, CharHeight);
	Vec2 CellSize(CellWidth, CellHeight);
	B->load_skin_font_image(Fn.c_str(), Size, CellSize, Size, startChar);
}

/// Font and string types. Instantiate a font with TruetypeFont() or LoadBitmapFont() - they are on the "Font" namespace.
/// @engineclass Strings
void CreateStringsLuaInterface(LuaManager* AnimLua)
{

	/// Base class for fonts.
	// @type Font.Font
	luabridge::getGlobalNamespace(AnimLua->get_lua_state())
		.beginNamespace("Fonts")
		.beginClass<Font>("Font")
		/// Set font color.
		// @function SetColor
		// @param r Red, from 0 to 1.
		// @param g Green, from 0 to 1.
		// @param b Blue, from 0 to 1.
		.addFunction("SetColor", &Font::set_color)
		/// Set font alpha
		// @function SetAlpha
		// @param alpha New alpha value.
		.addFunction("SetAlpha", &Font::set_alpha)
		/// Get horizontal length for a string.
		// @function GetLength
		// @tparam string s The string to get the length of.
		.addFunction("GetLength", &Font::get_horizontal_length)
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
	luabridge::getGlobalNamespace(AnimLua->get_lua_state())
		.deriveClass<GraphicalString, Sprite>("StringObject2D")
		/// Create a new instance of StringObject2D.
		// @function StringObject2D
		.addConstructor <void(*) ()>()
		/// Sets the font of this string.
		// @property Font
		.addProperty("Font", &GraphicalString::get_font, &GraphicalString::set_font)
		/// Sets the text of this string.
		// @property Text
		.addProperty("Text", &GraphicalString::get_text, &GraphicalString::set_text)
		/// Sets the font size for this string.
		// @property FontSize
		.addProperty("FontSize", &GraphicalString::get_font_size, &GraphicalString::set_font_size)
		/// Returns the length of the currently set string.
		// @property TextSize
		.addProperty("TextSize", &GraphicalString::get_text_size)
		/// Sets the kerning scale. 
		// @property KernScale
		.addProperty("KernScale", &GraphicalString::get_kerning_scale, &GraphicalString::set_kerning_scale)
		// .addProperty("ChainTransformation", &O2DProxy::getChainTransformation<GraphicalString>, &O2DProxy::setChainTransformation<GraphicalString>)
		.endClass();
}