#ifndef GOM_H_
#define GOM_H_

#include <limits>

class GraphObject2D;
class LuaManager;
class ImageList;

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
	std::vector<GraphObject2D*> Objects;
	std::vector <Animation> Animations;
	bool mFrameSkip;
public:
	SceneManager();
	~SceneManager();

	void Preload(GString Filename, GString ArrayName);
	void Initialize(GString Filename = "", bool RunScript = true);
	LuaManager *GetEnv();
	ImageList* GetImageList();

	void DoEvent(GString EventName, int Return = 0);
	void AddLuaAnimation (GraphObject2D* Target, const GString &FName, int Easing, float Duration, float Delay);
	void StopAnimationsForTarget(GraphObject2D* Target);
	void AddTarget(GraphObject2D *Targ);
	void AddLuaTarget(GraphObject2D *Targ, GString Varname);
	void AddLuaTargetArray(GraphObject2D *Targ, GString Varname, GString Arrname);
	void RemoveTarget(GraphObject2D *Targ);
	void DrawTargets(double TimeDelta);

	void Sort();

	void UpdateTargets(double TimeDelta);
	void DrawUntilLayer(uint32 Layer);
	void DrawFromLayer(uint32 Layer);

	void HandleInput(int32 key, KeyEventType code, bool isMouseInput);
};

#endif