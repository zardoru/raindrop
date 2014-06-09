
#ifndef GOBJ_H_
#define GOBJ_H_

#include "GraphObject2D.h"

class GameObject : public GraphObject2D
{
private:
	friend class ScreenEdit;

	bool BeingHeld;

	int32 heldKey;

	unsigned char AnimationStatus;

public:

	double startTime, endTime, beat, hold_duration;
	uint32 Measure;
	double Fraction;
	float fadeout_time, fadein_time; // time to fadeout, and time to get a hit
	float waiting_time;

	GameObject();
	static void GlobalInit();
	void Initialize();

	Judgement Hit(double time, Vec2 mpos, bool KeyDown, bool Autoplay, int32 Key);
	Judgement Run(double delta, double Time, bool Autoplay);
	void Animate(float delta, float songTime);
	void Assign(double Duration, uint32 Measure, double MeasureFraction);

	double GetFraction() const;
	void SetFraction (double frac);
	bool IsHold();
	void Invalidate();
	bool ShouldRemove();
};

typedef std::vector<GameObject> GameObjectVector;

#endif