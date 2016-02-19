#pragma once

namespace Configuration
{
    void Initialize();
	void Reload();
    std::string GetConfigs(std::string Name, std::string Namespace = "");
    float  GetConfigf(std::string Name, std::string Namespace = "");
    std::string GetSkinConfigs(std::string Name, std::string Namespace = "");
    double  GetSkinConfigf(std::string Name, std::string Namespace = "");

	std::filesystem::path GetSkinSound(std::string snd);
    void GetConfigListS(std::string Name, std::map<std::string, std::string> &Out, std::string DefaultKeyName);
    void GetConfigListS(std::string, std::map<std::string, std::filesystem::path>&, std::string);
    bool ListExists(std::string Name);

    bool HasTextureParameters(std::string filename);
    void LoadTextureParameters();
    std::string GetTextureParameter(std::string filename, std::string parameter);
    bool TextureParameterExists(std::string filename, std::string parameter);

    uint32_t CfgScreenHeight();
    uint32_t CfgScreenWidth();

    void SetConfig(std::string Name, std::string Value, std::string Namespace = "");
    // void SetConfig(std::string Name, float Value, std::string Namespace = "");
    /// void SaveConfig();
    void Cleanup();
}

#define ScreenHeight Configuration::CfgScreenHeight()
#define ScreenWidth Configuration::CfgScreenWidth()