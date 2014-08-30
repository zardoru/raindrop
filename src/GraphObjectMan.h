#ifndef GOM_H_
#define GOM_H_

class GraphObject2D;
class LuaManager;
class ImageList;

struct Animation {
	function <bool (GraphObject2D* Target, float Fraction)> Function;

	float Time, Duration;
	enum EEaseType {
		EaseLinear,
		EaseIn,
		EaseOut
	} Easing;

	GraphObject2D* Target;

	Animation() {
		Time = Duration = 0;
		Target = NULL;
	}
};

class GraphObjectMan
{
	LuaManager *Lua;
	ImageList *Images;
	std::vector<GraphObject2D*> Objects;
	std::vector <Animation> Animations;
public:
	GraphObjectMan();
	~GraphObjectMan();

	void Preload(String Filename, String ArrayName);
	void Initialize(String Filename = "", bool RunScript = true);
	LuaManager *GetEnv();
	ImageList* GetImageList();

	void AddLuaAnimation (GraphObject2D* Target, const String &FName, Animation::EEaseType Easing, float Duration);
	void AddTarget(GraphObject2D *Targ);
	void AddLuaTarget(GraphObject2D *Targ, String Varname);
	void AddLuaTargetArray(GraphObject2D *Targ, String Varname, String Arrname);
	void RemoveTarget(GraphObject2D *Targ);
	void DrawTargets(double TimeDelta);

	void UpdateTargets(double TimeDelta);
	void DrawUntilLayer(uint32 Layer);
	void DrawFromLayer(uint32 Layer);

	void HandleInput(int32 key, KeyEventType code, bool isMouseInput);
};

#endif