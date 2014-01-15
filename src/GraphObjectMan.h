#ifndef GOM_H_
#define GOM_H_

class GraphObject2D;
class LuaManager;

class GraphObjectMan
{
	LuaManager *Lua;
	std::vector<GraphObject2D*> Objects;
public:
	GraphObjectMan();
	~GraphObjectMan();

	void Initialize(String Filename);
	LuaManager *GetEnv();

	void AddTarget(GraphObject2D *Targ);
	void AddLuaTarget(GraphObject2D *Targ, String Varname);
	void RemoveTarget(GraphObject2D *Targ);
	void DrawTargets(double TimeDelta);

	void UpdateTargets(double TimeDelta);
	void DrawUntilLayer(uint32 Layer);
	void DrawFromLayer(uint32 Layer);

	void HandleInput(int32 key, KeyEventType code, bool isMouseInput);
};

#endif