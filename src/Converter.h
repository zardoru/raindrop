#pragma once

void ConvertToOM(Game::VSRG::Song *Sng, std::filesystem::path PathOut, std::string Author);
void ConvertToBMS(Game::VSRG::Song *Sng, std::filesystem::path PathOut);
void ConvertToSMTiming(Game::VSRG::Song *Sng, std::filesystem::path PathOut);
void ConvertToNPSGraph(Game::VSRG::Song *Sng, std::filesystem::path PathOut);