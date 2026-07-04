#pragma once

#include <ChartGroup.h>

class SongDatabase;

class SongLoader
{
    SongDatabase* DB;

public:
    SongLoader(SongDatabase* usedDatabase);

	void LoadBMS(
		std::shared_ptr<otoworm::ChartGroup> &bms_group,
		std::filesystem::path File,
		std::map<std::string, std::shared_ptr<otoworm::ChartGroup>> &bmsk,
		std::vector<std::shared_ptr<otoworm::ChartGroup>> & VecOut);

	void LoadChartGroupsFromDir(std::filesystem::path songPath, std::vector<std::shared_ptr<otoworm::ChartGroup>> &VecOut);
    void GetChartGroupList(std::vector<std::shared_ptr<otoworm::ChartGroup>> &OutVec, std::filesystem::path Dir);
    std::shared_ptr<otoworm::ChartGroup> LoadFromMeta(int meta_song_id, const std::shared_ptr<otoworm::Chart>& current_chart, std::filesystem::path& FilenameOut, uint8_t& Index);
};

std::shared_ptr<otoworm::ChartGroup> LoadChartGroupFromFilename(const std::filesystem::path& Filename);
