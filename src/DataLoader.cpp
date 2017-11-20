
#include "DataLoader.h"
#include <sstream>
#include <string>
#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <algorithm> 
#include <functional> 
#include <cctype>
#include <locale>

using namespace std;

DataLoader::DataLoader()
{
}


DataLoader::~DataLoader()
{
}

std::map<string, float>  DataLoader::loadAnnualCo2Data1850() {
	map<string, float> co2Data;
	ifstream input("data\\CO2_OBS_1850-2005.lpl.txt");
	if (!input)
		return co2Data;

	string year;
	float co2;
	for (string line; getline(input, line); )
	{
		if (line == "" || line == " x  CO2")
			continue;
		istringstream iss(line);
		iss >> year >> co2;
		co2Data[year] = co2;
	}
	return co2Data;
}

std::map<string, float>  DataLoader::loadMonthlySeaLevelData1880() {
	map<string, float> co2Data;
	ifstream input("data\\CSIRO_Recons_gmsl_mo_2015.txt");
	if (!input)
		return co2Data;

	float time;
	float seaLevel;
	float accuracy;
	for (string line; getline(input, line); )
	{
		istringstream iss(line);
		iss >> time >> seaLevel >> accuracy;
		char date[8];
		int year = time;
		int month = (time - (int)time) * 12 + 1;
		sprintf(date, "%04d-%02d", year, month);
		co2Data[date] = seaLevel;
	}
	return co2Data;
}

std::map<string, float> DataLoader::loadMonthlyCo2Data1958() {
	map<string, float> co2Data;
	ifstream input("data\\monthly_in_situ_co2_mlo.csv");
	if (!input)
		return co2Data;

	int year;
	int month;
	float co2;
	for (string line; getline(input, line); )
	{
		if (line[0] == '\"' || line[0] == ' ')
			continue;

		vector<string> values = split(line, ',');
		year = atoi(values.at(0).c_str());
		month = atoi(values.at(1).c_str());
		co2 = strtof(values.at(4).c_str(),0);
		char date[8];
		sprintf(date, "%04d-%02d", year, month);
		co2Data[date] = co2;
	}
	return co2Data;

}

std::map<string, float> DataLoader::loadMonthlyTemperature1880() {
	
	map<string, float> temperatureData;

	ifstream input("data\\GLB.Ts+dSST.csv");
	if (!input)
		return temperatureData;

	string year;
	float temperature;
	for (string line; getline(input, line); )
	{
		if (line[0] == 'L' || line[0] == 'Y')
			continue;

		vector<string> values = split(line, ',');
		int year = atoi(values.at(0).c_str());
		for (int month = 1; month <= 12; month++) {
			temperature = strtof(values.at(month).c_str(), 0);
			char date[8];
			sprintf(date, "%04d-%02d", year, month);
			temperatureData[date] = temperature;
		}

	}
	input.close();
	return temperatureData;
}

std::vector<std::string> DataLoader::split(const std::string &text, char sep) {
	std::vector<std::string> tokens;
	std::size_t start = 0, end = 0;
	while ((end = text.find(sep, start)) != std::string::npos) {
		tokens.push_back(trim(text.substr(start, end - start)));
		start = end + 1;
	}
	tokens.push_back(text.substr(start));
	return tokens;
}

// trim from start
std::string &DataLoader::ltrim(std::string &s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(),
		std::not1(std::ptr_fun<int, int>(std::isspace))));
	return s;
}

// trim from end
std::string &DataLoader::rtrim(std::string &s) {
	s.erase(std::find_if(s.rbegin(), s.rend(),
		std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
	return s;
}

// trim from both ends
std::string &DataLoader::trim(std::string &s) {
	return ltrim(rtrim(s));
}