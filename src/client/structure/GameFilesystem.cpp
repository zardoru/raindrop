#include "GameFilesystem.h"

#include "Configuration.h"
#include "TextAndFileUtil.h"

#include <fstream>
#include <utility>

namespace {
constexpr auto SkinsPrefix = "skins/";
constexpr auto ScriptsPrefix = "scripts/";
}

GameFilesystem::GameFilesystem()
{
    load_skin_fallbacks();
}

void GameFilesystem::load_skin_fallbacks()
{
    fallback.clear();

    const std::filesystem::path skins_dir = directory_prefix + SkinsPrefix;
    if (!std::filesystem::exists(skins_dir))
        return;

    const std::vector<std::filesystem::path> listing = Utility::GetFileListing(skins_dir);
    for (const auto& skin_path : listing) {
        auto skin = skin_path.filename().string();
        std::ifstream fallback_file(skin_path / "fallback.txt");
        if (fallback_file.is_open() && skin_path != "default") {
            std::string line;
            while (getline(fallback_file, line)) {
                if (Utility::ToLower(line) != Utility::ToLower(skin))
                    fallback[skin].push_back(line);
            }
        }
        if (fallback[skin].empty())
            fallback[skin].push_back("default");
    }
}

void GameFilesystem::set_system_folder(std::string folder)
{
    directory_prefix = std::move(folder);
    load_skin_fallbacks();
}

std::string GameFilesystem::get_directory_prefix() const
{
    return directory_prefix;
}

void GameFilesystem::set_skin(std::string skin)
{
    current_skin = std::move(skin);
}

std::string GameFilesystem::get_skin() const
{
    return current_skin;
}

std::string GameFilesystem::get_first_fallback_skin() const
{
    const auto found = fallback.find(current_skin);
    if (found != fallback.end() && !found->second.empty())
        return found->second.front();
    return "default";
}

std::string GameFilesystem::get_skin_prefix() const
{
    return get_skin_prefix(current_skin);
}

std::string GameFilesystem::get_skin_prefix(const std::string& skin) const
{
    return directory_prefix + SkinsPrefix + skin + "/";
}

std::string GameFilesystem::get_scripts_directory() const
{
    return directory_prefix + ScriptsPrefix;
}

bool GameFilesystem::file_exists_on_skin(const char* filename, const char* skin) const
{
    std::filesystem::path path(skin);
    path = std::filesystem::path(directory_prefix) / (std::filesystem::path(SkinsPrefix) / path / filename);
    return std::filesystem::exists(path);
}

std::filesystem::path GameFilesystem::get_skin_script_file(const char* filename, const std::string& skin) const
{
    std::string resolved = filename;
    if (resolved.find(".lua") == std::string::npos)
        resolved += ".lua";

    return get_skin_file(resolved, skin).replace_extension("");
}

std::filesystem::path GameFilesystem::get_skin_file(const std::string& name, const std::string& skin) const
{
    std::string test = get_skin_prefix(skin) + name;

    if (std::filesystem::exists(test))
        return test;

    const auto found = fallback.find(skin);
    if (found != fallback.end()) {
        for (const auto& fallback_skin : found->second) {
            if (file_exists_on_skin(name.c_str(), fallback_skin.c_str()))
                return get_skin_file(name, fallback_skin);
        }
    }

    return test;
}

std::filesystem::path GameFilesystem::get_skin_file(const std::string& name) const
{
    return get_skin_file(name, current_skin);
}

std::filesystem::path GameFilesystem::get_fallback_skin_file(const std::string& name) const
{
    const auto found = fallback.find(current_skin);
    if (found != fallback.end()) {
        for (const auto& fallback_skin : found->second) {
            if (file_exists_on_skin(name.c_str(), fallback_skin.c_str()))
                return get_skin_file(name, fallback_skin);
        }
    }

    return get_skin_prefix() + name;
}

bool GameFilesystem::skin_supports_channel_count(int count) const
{
    char list_name[256];
    snprintf(list_name, sizeof list_name, "Channels%d", count);
    return Configuration::ListExists(list_name);
}
