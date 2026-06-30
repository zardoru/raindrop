#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <vector>

class GameFilesystem {
    std::string directory_prefix = "data/";
    std::string current_skin = "default";
    std::map<std::string, std::vector<std::string>> fallback;

    bool file_exists_on_skin(const char* filename, const char* skin) const;
    void load_skin_fallbacks();

public:
    GameFilesystem();

    void set_system_folder(std::string folder);
    std::string get_directory_prefix() const;

    void set_skin(std::string skin);
    std::string get_skin() const;
    std::string get_first_fallback_skin() const;

    std::string get_skin_prefix() const;
    std::string get_skin_prefix(const std::string& skin) const;
    std::string get_scripts_directory() const;

    std::filesystem::path get_skin_script_file(const char* filename, const std::string& skin) const;
    std::filesystem::path get_skin_file(const std::string& name, const std::string& skin) const;
    std::filesystem::path get_skin_file(const std::string& name) const;
    std::filesystem::path get_fallback_skin_file(const std::string& name) const;

    bool skin_supports_channel_count(int count) const;
};
