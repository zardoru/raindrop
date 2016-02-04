#pragma once

void ConvertToOM(VSRG::Song *Sng, Directory PathOut, std::string Author);
void ConvertToBMS(VSRG::Song *Sng, Directory PathOut);
void ConvertToSMTiming(VSRG::Song *Sng, Directory PathOut);
void ConvertToNPSGraph(VSRG::Song *Sng, Directory PathOut);