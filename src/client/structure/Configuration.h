#pragma once

namespace Configuration
{
	void SetConfigFile(std::string cfgfile);
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

class ConfigurationVariable
{
protected:
	std::string nm, ns;
public:
	ConfigurationVariable(std::string _nm, std::string _ns = "") : nm(_nm), ns(_ns) {};
	~ConfigurationVariable() = default;
	virtual std::string str() const
	{
		return Configuration::GetConfigs(nm, ns);
	};

	virtual float flt() const
	{
		return Configuration::GetConfigf(nm, ns);
	}

	operator float() const
	{
		return flt();
	}

	explicit operator bool() const
	{
		return flt() != 0;
	}

	explicit operator int() const
	{
		return static_cast<int>(floor(flt()));
	}

	operator std::string() const
	{
		return str();
	}

	std::map<std::string, std::string> str_list() const
	{
		std::map<std::string, std::string> out;
		Configuration::GetConfigListS(nm, out, ns);
		return out;
	}
};

class SkinMetric : public ConfigurationVariable {
public:
	SkinMetric(std::string _nm, std::string _ns = "") : ConfigurationVariable(_nm, _ns) {}
	std::string str() const override
	{ 
		return Configuration::GetSkinConfigs(nm, ns);
	}

	float flt() const override
	{
		return Configuration::GetSkinConfigf(nm, ns);
	}
};

typedef ConfigurationVariable CfgVar;

#define ScreenHeight Configuration::CfgScreenHeight()
#define ScreenWidth Configuration::CfgScreenWidth()