#ifndef GOM_H_
#define GOM_H_

#include <limits>

class Drawable2D;
class GraphObject2D;
class LuaManager;
class ImageList;
class RocketContextObject;
class TruetypeFont;

namespace Rocket {
	namespace Core {
		class Context;
	}
}

struct Animation {
	function <bool (GraphObject2D* Target, float Fraction)> Function;

	float Time, Duration, Delay;
	enum EEaseType {
		EaseLinear,
		EaseIn,
		EaseOut
	} Easing;

	GraphObject2D* Target;

	Animation() {
		Time = Delay = 0;
		Duration = std::numeric_limits<float>::infinity();
		Target = NULL;
	}
};

class SceneManager
{
	LuaManager *Lua;
	ImageList *Images;
	std::vector<Drawable2D*> Objects;
	std::vector<Drawable2D*> ManagedObjects;
	std::vector<TruetypeFont*> ManagedFonts;
	std::vector <Animation> Animations;
	bool mFrameSkip;
	GString mScreenName;

	Rocket::Core::Context* ctx;
	RocketContextObject* obctx;
public:
	SceneManager(const char* ScreenName, bool initGUI = false);
	~SceneManager();

	void InitializeUI();
	void RunUIScript(GString Filename);
	void RunUIFunction(GString Funcname);
	void SetUILayer(uint32 Layer);
	void Preload(GString Filename, GString ArrayName);
	void Initialize(GString Filename = "", bool RunScript = true);
	LuaManager *GetEnv();
	ImageList* GetImageList();

	GraphObject2D* CreateObject();

	void DoEvent(GString EventName, int Return = 0);
	void AddLuaAnimation (GraphObject2D* Target, const GString &FName, int Easing, float Duration, float Delay);
	void StopAnimationsForTarget(GraphObject2D* Target);
	void AddTarget(GraphObject2D *Targ);
	void AddLuaTarget(GraphObject2D *Targ, GString Varname);
	void AddLuaTargetArray(GraphObject2D *Targ, GString Varname, GString Arrname);
	void RemoveTarget(GraphObject2D *Targ);
	void DrawTargets(double TimeDelta);

	TruetypeFont* CreateTTF(const char* Dir, float Size);

	void Sort();

	void UpdateTargets(double TimeDelta);
	void DrawUntilLayer(uint32 Layer);
	void DrawFromLayer(uint32 Layer);

	bool HandleInput(int32 key, KeyEventType code, bool isMouseInput);
	bool HandleTextInput(int codepoint);
	bool IsManagedObject(Drawable2D *Obj);
	void StopManagingObject(Drawable2D *Obj);
	void RemoveManagedObject(Drawable2D *Obj);
};

#endif