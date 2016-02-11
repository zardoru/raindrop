#pragma once

void ConvertToOM(VSRG::Song *Sng, std::filesystem::path PathOut, std::string Author);
void ConvertToBMS(VSRG::Song *Sng, std::filesystem::path PathOut);
void ConvertToSMTiming(VSRG::Song *Sng, std::filesystem::path PathOut);
void ConvertToNPSGraph(VSRG::Song *Sng, std::filesystem::path PathOut);