#include "pch.h"
#include "Easing.h"

std::function<float(float)> ease_none = passthrough;
std::function<float(float)> ease_in = pow_ease_in<2>;
std::function<float(float)> ease_out = pow_ease_out<2>;
std::function<float(float)> ease_inout = pow_ease_inout<2>;
std::function<float(float)> ease_3in = pow_ease_in<3>;
std::function<float(float)> ease_3out = pow_ease_out<3>;
std::function<float(float)> ease_3inout = pow_ease_inout<3>;
std::function<float(float)> ease_4in = pow_ease_in<4>;
std::function<float(float)> ease_4out = pow_ease_out<4>;
std::function<float(float)> ease_4inout = pow_ease_inout<4>;
std::function<float(float)> ease_5in = pow_ease_in<5>;
std::function<float(float)> ease_5out = pow_ease_out<5>;
std::function<float(float)> ease_5inout = pow_ease_inout<5>;


