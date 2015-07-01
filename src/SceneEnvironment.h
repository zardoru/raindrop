#ifndef GOM_H_
#define GOM_H_

#include <limits>

class Drawable2D;
class Sprite;
class LuaManager;
class ImageList;
class RocketContextObject;
class TruetypeFont;

namespace Rocket {
	namespace Core {
		class Context;
		class ElementDocument;
	}
}

struct Animation {
	function <bool (float Fraction)> Function;

	float Time, Duration, Delay;
	enum EEaseType {
		EaseLinear,
		EaseIn,
		EaseOut
	} Easing;

	Sprite* Target;

	Animation() {
		Time = Delay = 0;
		Duration = std::numeric_limits<float>::infinity();
		Target = NULL;
	}
};

class SceneEnvironment
{
	LuaManager *Lua;
	ImageList *Images;
	vector<Drawable2D*> Objects;
	vector<Drawable2D*> ManagedObjects;
	vector<Drawable2D*> ExternalObjects;
	vector<TruetypeFont*> ManagedFonts;
	vector <Animation> Animations;
	bool mFrameSkip;
	GString mScreenName;
	GString mInitScript;

	Rocket::Core::Context* ctx;
	Rocket::Core::ElementDocument *Doc;
	RocketContextObject* obctx;
public:
	SceneEnvironment(const char* ScreenName, bool initGUI = false);
	~SceneEnvironment();

	void RemoveManagedObjects();
	void RemoveExternalObjects();

	void InitializeUI();

	void ReloadScripts();
	void ReloadUI();
	void ReloadAll();

	void RunUIScript(GString Filename);
	void RunUIFunction(GString Funcname);
	void SetUILayer(uint32 Layer);
	void Preload(GString Filename, GString ArrayName);
	void Initialize(GString Filename = "", bool RunScript = true);
	LuaManager *GetEnv();
	ImageList* GetImageList();

	Sprite* CreateObject();

	void DoEvent(GString EventName, int Return = 0);
	void AddLuaAnimation (Sprite* Target, const GString &FName, int Easing, float Duration, float Delay);
	void StopAnimationsForTarget(Sprite* Target);
	void AddTarget(Sprite *Targ, bool IsExternal = false);
	void AddLuaTarget(Sprite *Targ, GString Varname);
	void AddLuaTargetArray(Sprite *Targ, GString Varname, GString Arrname);
	void RemoveTarget(Drawable2D *Targ);
	void DrawTargets(double TimeDelta);

	TruetypeFont* CreateTTF(const char* Dir, float Size);

	void Sort();

	void UpdateTargets(double TimeDelta);
	void DrawUntilLayer(uint32 Layer);
	void DrawFromLayer(uint32 Layer);

	void RunIntro(float Fraction, float Delta);
	void RunExit(float Fraction, float Delta);

	float GetIntroDuration();
	float GetExitDuration();

	bool HandleInput(int32 key, KeyEventType code, bool isMouseInput);
	bool HandleTextInput(int codepoint);
	bool IsManagedObject(Drawable2D *Obj);
	void StopManagingObject(Drawable2D *Obj);
	void RemoveManagedObject(Drawable2D *Obj);
};

void DefineSpriteInterface(LuaManager* anim_lua);

#endif