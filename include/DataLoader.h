#pragma once
#include <map>
#include <vector>
class DataLoader
{
public:
	DataLoader();
	~DataLoader();
	static std::map<std::string, float> loadAnnualCo2Data1850();
	static std::map<std::string, float> loadMonthlySeaLevelData1880();
	static std::map<std::string, float> loadMonthlyCo2Data1958();
	static std::map<std::string, float> loadMonthlyTemperature1880();
private:
	static inline std::string &rtrim(std::string &s);
	static inline std::string &trim(std::string &s);
	static inline std::string &ltrim(std::string &s);
	static inline std::vector<std::string> split(const std::string &text, char sep);
};

