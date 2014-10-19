#ifndef CMAN_H_
#define CMAN_H_

namespace Configuration
{
	void Initialize();
	String GetConfigs(String Name, String Namespace = "");
	float  GetConfigf(String Name, String Namespace = "");
	String GetSkinConfigs(String Name, String Namespace = "");
	double  GetSkinConfigf(String Name, String Namespace = "");
	void GetConfigListS(String Name, std::vector<String> &Out);
	bool ListExists(String Name);

	double CfgScreenHeight();
	double CfgScreenWidth();

	// void SetConfig(String Name, String Value, String Namespace = "");
	// void SetConfig(String Name, float Value, String Namespace = "");
	/// void SaveConfig();
	void Cleanup();
}

#define ScreenHeight Configuration::CfgScreenHeight()
#define ScreenWidth Configuration::CfgScreenWidth()

#endif