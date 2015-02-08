#include <Rocket/Core.h>
#include <Rocket/Core/Lua/Interpreter.h>
#include <GLFW/glfw3.h>

#include "GameGlobal.h"
#include "GameState.h"
#include "GraphObject2D.h"
#include "LuaManager.h"
#include "SceneManager.h"
#include "ImageList.h"
#include "Logging.h"
#include "GameWindow.h"

#include "RaindropRocketInterface.h"
#include "TruetypeFont.h"

void CreateLuaInterface(LuaManager *AnimLua);

std::map<int, Rocket::Core::Input::KeyIdentifier> key_identifier_map;

void assignKeyMap()
{
	static bool initialized = false;
	if (initialized) return;

	// Assign individual values.
	key_identifier_map['A'] = Rocket::Core::Input::KI_A;
	key_identifier_map['B'] = Rocket::Core::Input::KI_B;
	key_identifier_map['C'] = Rocket::Core::Input::KI_C;
	key_identifier_map['D'] = Rocket::Core::Input::KI_D;
	key_identifier_map['E'] = Rocket::Core::Input::KI_E;
	key_identifier_map['F'] = Rocket::Core::Input::KI_F;
	key_identifier_map['G'] = Rocket::Core::Input::KI_G;
	key_identifier_map['H'] = Rocket::Core::Input::KI_H;
	key_identifier_map['I'] = Rocket::Core::Input::KI_I;
	key_identifier_map['J'] = Rocket::Core::Input::KI_J;
	key_identifier_map['K'] = Rocket::Core::Input::KI_K;
	key_identifier_map['L'] = Rocket::Core::Input::KI_L;
	key_identifier_map['M'] = Rocket::Core::Input::KI_M;
	key_identifier_map['N'] = Rocket::Core::Input::KI_N;
	key_identifier_map['O'] = Rocket::Core::Input::KI_O;
	key_identifier_map['P'] = Rocket::Core::Input::KI_P;
	key_identifier_map['Q'] = Rocket::Core::Input::KI_Q;
	key_identifier_map['R'] = Rocket::Core::Input::KI_R;
	key_identifier_map['S'] = Rocket::Core::Input::KI_S;
	key_identifier_map['T'] = Rocket::Core::Input::KI_T;
	key_identifier_map['U'] = Rocket::Core::Input::KI_U;
	key_identifier_map['V'] = Rocket::Core::Input::KI_V;
	key_identifier_map['W'] = Rocket::Core::Input::KI_W;
	key_identifier_map['X'] = Rocket::Core::Input::KI_X;
	key_identifier_map['Y'] = Rocket::Core::Input::KI_Y;
	key_identifier_map['Z'] = Rocket::Core::Input::KI_Z;

	key_identifier_map['0'] = Rocket::Core::Input::KI_0;
	key_identifier_map['1'] = Rocket::Core::Input::KI_1;
	key_identifier_map['2'] = Rocket::Core::Input::KI_2;
	key_identifier_map['3'] = Rocket::Core::Input::KI_3;
	key_identifier_map['4'] = Rocket::Core::Input::KI_4;
	key_identifier_map['5'] = Rocket::Core::Input::KI_5;
	key_identifier_map['6'] = Rocket::Core::Input::KI_6;
	key_identifier_map['7'] = Rocket::Core::Input::KI_7;
	key_identifier_map['8'] = Rocket::Core::Input::KI_8;
	key_identifier_map['9'] = Rocket::Core::Input::KI_9;

	key_identifier_map[GLFW_KEY_BACKSPACE] = Rocket::Core::Input::KI_BACK;
	key_identifier_map[GLFW_KEY_TAB] = Rocket::Core::Input::KI_TAB;

	key_identifier_map[GLFW_KEY_ENTER] = Rocket::Core::Input::KI_RETURN;

	key_identifier_map[GLFW_KEY_PAUSE] = Rocket::Core::Input::KI_PAUSE;
	key_identifier_map[GLFW_KEY_CAPS_LOCK] = Rocket::Core::Input::KI_CAPITAL;

	key_identifier_map[GLFW_KEY_ESCAPE] = Rocket::Core::Input::KI_ESCAPE;

	key_identifier_map[GLFW_KEY_SPACE] = Rocket::Core::Input::KI_SPACE;
	key_identifier_map[GLFW_KEY_END] = Rocket::Core::Input::KI_END;
	key_identifier_map[GLFW_KEY_HOME] = Rocket::Core::Input::KI_HOME;
	key_identifier_map[GLFW_KEY_LEFT] = Rocket::Core::Input::KI_LEFT;
	key_identifier_map[GLFW_KEY_UP] = Rocket::Core::Input::KI_UP;
	key_identifier_map[GLFW_KEY_RIGHT] = Rocket::Core::Input::KI_RIGHT;
	key_identifier_map[GLFW_KEY_DOWN] = Rocket::Core::Input::KI_DOWN;
	key_identifier_map[GLFW_KEY_INSERT] = Rocket::Core::Input::KI_INSERT;
	key_identifier_map[GLFW_KEY_DELETE] = Rocket::Core::Input::KI_DELETE;

	key_identifier_map[GLFW_KEY_KP_0] = Rocket::Core::Input::KI_NUMPAD0;
	key_identifier_map[GLFW_KEY_KP_1] = Rocket::Core::Input::KI_NUMPAD1;
	key_identifier_map[GLFW_KEY_KP_2] = Rocket::Core::Input::KI_NUMPAD2;
	key_identifier_map[GLFW_KEY_KP_3] = Rocket::Core::Input::KI_NUMPAD3;
	key_identifier_map[GLFW_KEY_KP_4] = Rocket::Core::Input::KI_NUMPAD4;
	key_identifier_map[GLFW_KEY_KP_5] = Rocket::Core::Input::KI_NUMPAD5;
	key_identifier_map[GLFW_KEY_KP_6] = Rocket::Core::Input::KI_NUMPAD6;
	key_identifier_map[GLFW_KEY_KP_7] = Rocket::Core::Input::KI_NUMPAD7;
	key_identifier_map[GLFW_KEY_KP_8] = Rocket::Core::Input::KI_NUMPAD8;
	key_identifier_map[GLFW_KEY_KP_9] = Rocket::Core::Input::KI_NUMPAD9;
	key_identifier_map[GLFW_KEY_KP_MULTIPLY] = Rocket::Core::Input::KI_MULTIPLY;
	key_identifier_map[GLFW_KEY_KP_ADD] = Rocket::Core::Input::KI_ADD;
	key_identifier_map[GLFW_KEY_KP_SUBTRACT] = Rocket::Core::Input::KI_SUBTRACT;
	key_identifier_map[GLFW_KEY_KP_DECIMAL] = Rocket::Core::Input::KI_DECIMAL;
	key_identifier_map[GLFW_KEY_KP_DIVIDE] = Rocket::Core::Input::KI_DIVIDE;
	key_identifier_map[GLFW_KEY_F1] = Rocket::Core::Input::KI_F1;
	key_identifier_map[GLFW_KEY_F2] = Rocket::Core::Input::KI_F2;
	key_identifier_map[GLFW_KEY_F3] = Rocket::Core::Input::KI_F3;
	key_identifier_map[GLFW_KEY_F4] = Rocket::Core::Input::KI_F4;
	key_identifier_map[GLFW_KEY_F5] = Rocket::Core::Input::KI_F5;
	key_identifier_map[GLFW_KEY_F6] = Rocket::Core::Input::KI_F6;
	key_identifier_map[GLFW_KEY_F7] = Rocket::Core::Input::KI_F7;
	key_identifier_map[GLFW_KEY_F8] = Rocket::Core::Input::KI_F8;
	key_identifier_map[GLFW_KEY_F9] = Rocket::Core::Input::KI_F9;
	key_identifier_map[GLFW_KEY_F10] = Rocket::Core::Input::KI_F10;
	key_identifier_map[GLFW_KEY_F11] = Rocket::Core::Input::KI_F11;
	key_identifier_map[GLFW_KEY_F12] = Rocket::Core::Input::KI_F12;
	key_identifier_map[GLFW_KEY_F13] = Rocket::Core::Input::KI_F13;
	key_identifier_map[GLFW_KEY_F14] = Rocket::Core::Input::KI_F14;
	key_identifier_map[GLFW_KEY_F15] = Rocket::Core::Input::KI_F15;
	key_identifier_map[GLFW_KEY_F16] = Rocket::Core::Input::KI_F16;
	key_identifier_map[GLFW_KEY_F17] = Rocket::Core::Input::KI_F17;
	key_identifier_map[GLFW_KEY_F18] = Rocket::Core::Input::KI_F18;
	key_identifier_map[GLFW_KEY_F19] = Rocket::Core::Input::KI_F19;
	key_identifier_map[GLFW_KEY_F20] = Rocket::Core::Input::KI_F20;
	key_identifier_map[GLFW_KEY_F21] = Rocket::Core::Input::KI_F21;
	key_identifier_map[GLFW_KEY_F22] = Rocket::Core::Input::KI_F22;
	key_identifier_map[GLFW_KEY_F23] = Rocket::Core::Input::KI_F23;
	key_identifier_map[GLFW_KEY_F24] = Rocket::Core::Input::KI_F24;

	key_identifier_map[GLFW_KEY_NUM_LOCK] = Rocket::Core::Input::KI_NUMLOCK;
	key_identifier_map[GLFW_KEY_SCROLL_LOCK] = Rocket::Core::Input::KI_SCROLL;

	key_identifier_map[GLFW_KEY_LEFT_SHIFT] = Rocket::Core::Input::KI_LSHIFT;
	key_identifier_map[GLFW_KEY_LEFT_CONTROL] = Rocket::Core::Input::KI_LCONTROL;
	key_identifier_map[GLFW_KEY_MENU] = Rocket::Core::Input::KI_LMENU;

	initialized = true;
}

class RocketContextObject : public Drawable2D {
public:
	Rocket::Core::Context * ctx;
	void Render()
	{
		if (ctx)
			ctx->Render();
	}
};

bool LuaAnimation(LuaManager* Lua, GString Func, GraphObject2D* Target, float Frac)
{
	Lua->RegisterStruct( "Target", (void*)Target );
	if (Lua->CallFunction(Func.c_str(), 1, 1))
	{
		Lua->PushArgument(Frac);

		if (Lua->RunFunction())
			return Lua->GetFunctionResult() > 0;
		else
			return false;
	}
	else return false;
}

void SceneManager::StopAnimationsForTarget(GraphObject2D* Target)
{
	for (auto i = Animations.begin();
		i != Animations.end();
		)
	{
		if (i->Target == Target)
		{
			i = Animations.erase(i);
			if (i == Animations.end()) break;
			else continue;
		}
		
		i++;
	}
}


void SceneManager::AddLuaAnimation (GraphObject2D* Target, const GString &FuncName, 
	int Easing, float Duration, float Delay)
{
	Animation Anim;
	Anim.Function = boost::bind(LuaAnimation, Lua, FuncName, _1, _2);
	Anim.Easing = (Animation::EEaseType)Easing;
	Anim.Duration = Duration;
	Anim.Target = Target;
	Anim.Delay = Delay;

	Animations.push_back(Anim);
}

SceneManager::SceneManager(const char* ScreenName, bool initUI)
{
	Animations.reserve(10);
	Lua = new LuaManager;
	Lua->RegisterStruct("GOMAN", this);

	CreateLuaInterface(Lua);
	Images = new ImageList(true);
	mFrameSkip = true;

	ctx = NULL;
	obctx = NULL;
	mScreenName = ScreenName;

	assignKeyMap();

	if (initUI)
		InitializeUI();
}

TruetypeFont* SceneManager::CreateTTF(const char* Dir, float Size)
{
	TruetypeFont *Ret = new TruetypeFont(Dir, Size);
	ManagedFonts.push_back(Ret);
	return Ret;
}

void SceneManager::InitializeUI()
{
	RocketContextObject *Obj = new RocketContextObject();

	// Set up context
	ctx = Rocket::Core::CreateContext(mScreenName.c_str(), Rocket::Core::Vector2i(ScreenWidth, ScreenHeight));
	if (!ctx)
		ctx = Rocket::Core::GetContext(mScreenName.c_str());

	Obj->ctx = ctx;
	obctx = Obj;

	// Now set up document
	GString FName = mScreenName + GString(".rml");

	Rocket::Core::ElementDocument *Doc = ctx->LoadDocument(FName.c_str());
	if (Doc)
		Doc->Show();
	else
		Log::Printf("ScreenManager(): %s not found.\n", FName.c_str());

	ManagedObjects.push_back(obctx);
	Objects.push_back(obctx);
	SetUILayer(0);

	ctx->LoadMouseCursor("cursor.rml");	
}

void SceneManager::RunUIScript(GString Filename)
{
	lua_State* L = Rocket::Core::Lua::Interpreter::GetLuaState();
	luaL_dofile(L, GameState::GetInstance().GetSkinFile(Filename).c_str());
}

SceneManager::~SceneManager()
{
	if (Lua->CallFunction("Cleanup"))
	{
		Lua->RunFunction();
	}

	// Remove all managed drawable objects.
	for (auto i : ManagedObjects)
		delete i;

	for (auto i : ManagedFonts)
		delete i;

	ManagedObjects.clear();
	ManagedFonts.clear();

	if (ctx && ctx->GetReferenceCount() > 0)
		ctx->RemoveReference();

	delete Lua;
	delete Images;
}

void SceneManager::SetUILayer(uint32 Layer)
{
	obctx->SetZ(Layer);
	Sort();
}

void SceneManager::Preload(GString Filename, GString Arrname)
{
	Lua->RunScript(Filename);

	if (Lua->UseArray(Arrname))
	{
		Lua->StartIteration();

		while (Lua->IterateNext())
		{
			Images->AddToList (GameState::GetInstance().GetSkinFile(Lua->NextGString()), "");
			Lua->Pop();
		}

		Lua->Pop();
	}
}

void SceneManager::Sort()
{
	std::stable_sort(Objects.begin(), Objects.end(), 
		[](const Drawable2D *A, const Drawable2D *B)->bool  { return A->GetZ() < B->GetZ(); });
}

GraphObject2D* SceneManager::CreateObject()
{
	GraphObject2D* Out = new GraphObject2D;
	ManagedObjects.push_back(Out);
	AddTarget(Out);
	return Out;
}

bool SceneManager::IsManagedObject(Drawable2D *Obj)
{
	for (auto i: ManagedObjects)
	{
		if (Obj == i)
			return true;
	}

	return false;
}

void SceneManager::Initialize(GString Filename, bool RunScript)
{
	if (RunScript)
		Lua->RunScript(Filename);

	if (Lua->CallFunction("Init"))
		Lua->RunFunction();

	Images->LoadAll();
}

void SceneManager::AddTarget(GraphObject2D *Targ)
{
	Objects.push_back(Targ);
	Sort();
}

void SceneManager::AddLuaTarget(GraphObject2D *Targ, GString Varname)
{
	lua_State *L = Lua->GetState();
	GraphObject2D **RetVal = (GraphObject2D**) lua_newuserdata(L, sizeof(GraphObject2D **));
	*RetVal = Targ;
	luaL_getmetatable(L, "Sys.GraphObject2D");
	lua_setmetatable(L, -2);
	lua_setglobal(L, Varname.c_str());
}

void SceneManager::StopManagingObject(Drawable2D *Obj)
{
	for (auto i = ManagedObjects.begin(); i != ManagedObjects.end(); ++i)
	{
		if (Obj == *i)
		{
			ManagedObjects.erase(i);
			return;
		}
	}
}

void SceneManager::RemoveManagedObject(Drawable2D *Obj)
{
	for (auto i = ManagedObjects.begin(); i != ManagedObjects.end(); ++i)
	{
		if (*i == Obj)
		{
			delete *i;
			ManagedObjects.erase(i);
			return;
		}
	}
}

void SceneManager::RemoveTarget(GraphObject2D *Targ)
{
	StopManagingObject(Targ);

	for (auto i = Objects.begin(); i != Objects.end(); )
	{
		if (*i == Targ)
		{
			Objects.erase(i);
			continue;
		}

		if (i == Objects.end())
			break;

		++i;
	}
}

void SceneManager::DrawTargets(double TimeDelta)
{
	UpdateTargets(TimeDelta);

	DrawFromLayer(0);
}

void SceneManager::UpdateTargets(double TimeDelta)
{
	if (mFrameSkip)
	{
		mFrameSkip = false;
		return;
	}

	for (auto i = Animations.begin();
		i != Animations.end();)
	{
		if (i->Delay > 0)
		{
			i->Delay -= TimeDelta; // Still waiting for this to start.

			if (i->Delay < 0) // We rolled into the negatives.
				i->Time += -i->Delay; // Add it to passed time, to pretend it started right on time.
			else
			{
				i++;
				continue; // It hasn't began yet, so keep at it.
			}

		}else 
			i->Time += TimeDelta;

		if (i->Time >= i->Duration) // The animation is done. Call the function one last time with value 1 so it's completed.
		{
			i->Function(i->Target, 1);
			i = Animations.erase(i);
			if (i == Animations.end()) break;
			else continue;
		}

		float frac;

		switch (i->Easing) {
		case Animation::EaseIn:
			frac = pow(i->Time / i->Duration, 2);
			break;
		case Animation::EaseOut:
			frac = i->Time / i->Duration;
			frac = -frac*(frac-2);
			break;
		case Animation::EaseLinear:
		default:
			frac = i->Time / i->Duration;
		}

		if (!i->Function (i->Target, frac)) // Says the animation is over?
		{
			i = Animations.erase(i);
			if (i == Animations.end()) break;
			else continue;
		}

		i++;
	}

	if (Lua->CallFunction("Update", 1))
	{
		Lua->PushArgument(TimeDelta);
		Lua->RunFunction();
	}

	if (ctx)
	{
		Vec2 nMousePos = GameState::GetInstance().GetWindow()->GetRelativeMPos();
		ctx->ProcessMouseMove(nMousePos.x, nMousePos.y, 0);

		ctx->Update();
	}
}

void SceneManager::DrawUntilLayer(uint32 Layer)
{
	for (auto i = Objects.begin(); i != Objects.end(); ++i)
	{
		if (*i == NULL){ /* throw an error */ continue; }
		if ((*i)->GetZ() <= Layer)
			(*i)->Render();
	}
}

void SceneManager::DrawFromLayer(uint32 Layer)
{
	for (auto i = Objects.begin(); i != Objects.end(); ++i)
	{
		if ((*i)->GetZ() >= Layer)
			(*i)->Render();
	}
}

void InitializeUI()
{

}

LuaManager *SceneManager::GetEnv()
{
	return Lua;
}

bool SceneManager::HandleInput(int32 key, KeyEventType code, bool isMouseInput)
{
	if (Lua->CallFunction("KeyEvent", 3))
	{
		Lua->PushArgument(key);
		Lua->PushArgument(code);
		Lua->PushArgument(isMouseInput);
		Lua->RunFunction();
	}

	if (isMouseInput)
	{
		if (code == KE_Press)
		{
			if (ctx)
				ctx->ProcessMouseButtonDown(key, 0);
		}
		else if (code == KE_Release)
		{
			if (ctx)
				ctx->ProcessMouseButtonUp(key, 0);
		}
	}
	else {
		if (code == KE_Press)
		{
			if (ctx)
				ctx->ProcessKeyDown(key_identifier_map[key], 0);
		}
		else
		{
			if (ctx)
				ctx->ProcessKeyUp(key_identifier_map[key], 0);
		}
	}

	return true;
}

bool SceneManager::HandleTextInput(int codepoint)
{
	return ctx->ProcessTextInput(codepoint);
}

ImageList* SceneManager::GetImageList()
{
	return Images;
}

void SceneManager::DoEvent(GString EventName, int Return)
{
	if (Lua->CallFunction(EventName.c_str(), 0, Return))
		Lua->RunFunction();
}