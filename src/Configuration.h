#pragma once

namespace Configuration
{
	void Initialize();
	GString GetConfigs(GString Name, GString Namespace = "");
	float  GetConfigf(GString Name, GString Namespace = "");
	GString GetSkinConfigs(GString Name, GString Namespace = "");
	double  GetSkinConfigf(GString Name, GString Namespace = "");
	void GetConfigListS(GString Name, std::map<GString, GString> &Out, GString DefaultKeyName);
    void GetConfigListS(GString, std::map<GString, std::filesystem::path>&, GString);
	bool ListExists(GString Name);

	bool HasTextureParameters(GString filename);
	void LoadTextureParameters();
	GString GetTextureParameter(GString filename, GString parameter);
	bool TextureParameterExists(GString filename, GString parameter);

	double CfgScreenHeight();
	double CfgScreenWidth();

	void SetConfig(GString Name, GString Value, GString Namespace = "");
	// void SetConfig(GString Name, float Value, GString Namespace = "");
	/// void SaveConfig();
	void Cleanup();
}

#define ScreenHeight Configuration::CfgScreenHeight()
#define ScreenWidth Configuration::CfgScreenWidth()