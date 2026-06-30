#pragma once

class TTFCache {
	std::map<int, std::vector<uint8_t>> mCharBuffer;
public:
	TTFCache();
	bool load_cache(std::filesystem::path cachepath);
	bool save_cache(std::filesystem::path cachepath);
	const uint8_t* const get_character_buffer(int id);
	void set_character_buffer(int id, const uint8_t *data, size_t size);
};