#pragma once

// Algorithm to convert shape into SDF.
// Input: UNSIGNED BYTE texture (W * H)
// Output: float GL_ALPHA texture (w * h size)
void ConvertToSDF(unsigned char* out, unsigned char* tex, int w, int h);