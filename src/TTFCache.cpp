#include "pch.h"
#include "TTFCache.h"

TTFCache::TTFCache()
{
}


bool TTFCache::LoadCache(std::filesystem::path cachepath)
{
	std::ifstream in(cachepath.string(), std::ios::binary);
	if (!in.is_open()) return false;

	mCharBuffer.clear();
	int size;

	BinRead(in, size);
	while (!in.eof() && size > 0) {
		size_t chsize;
		int id;
		BinRead(in, id);
		BinRead(in, chsize);

		std::vector<uint8_t> buf(chsize);
		in.read((char*)buf.data(), chsize);

		mCharBuffer[id] = std::move(buf);
		size--;
	}

	return true;
}

bool TTFCache::SaveCache(std::filesystem::path cachepath)
{
	std::ofstream out(cachepath.string(), std::ios::binary);
	if (!out.is_open()) return false;

	auto size = mCharBuffer.size();
	BinWrite(out, size);

	for (const auto &it : mCharBuffer) {
		BinWrite(out, it.first);
		
		auto csize = it.second.size();
		BinWrite(out, csize);

		out.write((char*)it.second.data(), it.second.size());
	}

	return true;
}

const uint8_t * const TTFCache::GetCharacterBuffer(int id)
{
	if (mCharBuffer.find(id) != mCharBuffer.end()) {
		return mCharBuffer[id].data();
	}

	return nullptr;
}

void TTFCache::SetCharacterBuffer(int id, uint8_t * data, size_t size)
{
	if (mCharBuffer.find(id) != mCharBuffer.end()) {

	}
}
