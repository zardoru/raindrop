#ifndef CMAN_H_
#define CMAN_H_

#include <map>

namespace Configuration
{
	void Initialize();
	GString GetConfigs(GString Name, GString Namespace = "");
	float  GetConfigf(GString Name, GString Namespace = "");
	GString GetSkinConfigs(GString Name, GString Namespace = "");
	double  GetSkinConfigf(GString Name, GString Namespace = "");
	void GetConfigListS(GString Name, std::map<GString, GString> &Out, GString DefaultKeyName);
	bool ListExists(GString Name);

	double CfgScreenHeight();
	double CfgScreenWidth();

	// void SetConfig(GString Name, GString Value, GString Namespace = "");
	// void SetConfig(GString Name, float Value, GString Namespace = "");
	/// void SaveConfig();
	void Cleanup();
}

#define ScreenHeight Configuration::CfgScreenHeight()
#define ScreenWidth Configuration::CfgScreenWidth()

#endif