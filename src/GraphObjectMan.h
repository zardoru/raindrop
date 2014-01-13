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

	void AddTarget(GraphObject2D *Targ);
	void RemoveTarget(GraphObject2D *Targ);
	void DrawTargets(double TimeDelta);
};

#endif