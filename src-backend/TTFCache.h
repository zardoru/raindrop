#pragma once

class TTFCache {
	std::map<int, std::vector<uint8_t>> mCharBuffer;
public:
	TTFCache();
	bool LoadCache(std::filesystem::path cachepath);
	bool SaveCache(std::filesystem::path cachepath);
	const uint8_t* const GetCharacterBuffer(int id);
	void SetCharacterBuffer(int id, uint8_t *data, size_t size);
};