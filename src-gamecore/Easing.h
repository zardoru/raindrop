#pragma once

#include <math.h>

/*
	The least redundant way of defining easings, semantically speaking!
*/
inline float func_ease_in(float lerp, std::function<float(float)> f)
{
	return f(lerp);
}

inline float func_ease_out(float lerp, std::function<float(float)> f)
{
	return 1 - f(1 - lerp);
}

inline float f2_ease_inout(float lerp, std::function<float(float)> f1, std::function<float(float)> f2)
{
	return lerp * func_ease_in(lerp, f1) + (1 - lerp) * func_ease_out(lerp, f2);
}

inline float func_ease_inout(float lerp, std::function<float(float)> f1)
{
	return f2_ease_inout(lerp, f1, f1);
}

template<int v>
float pow_ease_in(float lerp) 
{
	auto f = [](float l) -> float { return pow(l, v); };
	return func_ease_in(lerp, f);
}

template<int v>
float pow_ease_out(float lerp)
{
	auto f = [](float l) -> float { return pow(l, v); };
	return func_ease_out(lerp, f);
}

template<int v>
float pow_ease_inout(float lerp)
{
	auto f = [](float l) -> float { return pow(l, v); };
	return func_ease_inout(lerp, f);
}

inline float passthrough(float v) { return v; }


extern std::function<float(float)> ease_none;
extern std::function<float(float)> ease_in;
extern std::function<float(float)> ease_out;
extern std::function<float(float)> ease_inout;
extern std::function<float(float)> ease_3in;
extern std::function<float(float)> ease_3out;
extern std::function<float(float)> ease_3inout;
extern std::function<float(float)> ease_4in;
extern std::function<float(float)> ease_4out;
extern std::function<float(float)> ease_4inout;
extern std::function<float(float)> ease_5in;
extern std::function<float(float)> ease_5out;
extern std::function<float(float)> ease_5inout;



