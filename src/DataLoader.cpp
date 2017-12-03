
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

	input.close();


	// The data in sl_global.txt is sampled every ~10 days so we take a monthly average.

	ifstream input2("data\\sl_global.txt");
	int previousMonth = 12;
	int valuesInMonth = 0;
	float accumSeaLevel = 0.0f;
	for (string line; getline(input2, line);) {
		if (line[0] == 'y')
			continue;

		istringstream iss(line);
		float time2;
		iss >> time2 >> seaLevel;

		// Skip overlap from CSIRO_Recons_gmsl_mo_2015.txt
		if (time2 < time)
			continue;

		int month = (time2 - (int)time2) * 12 + 1;
		if (month == previousMonth) {
			valuesInMonth++;
			accumSeaLevel += seaLevel;
			continue;
		}
		
		float averageSeaLevel = accumSeaLevel / valuesInMonth;

		char date[8];
		int year = time2;
		
		if (previousMonth == 12)
			year--;

		sprintf(date, "%04d-%02d", year, previousMonth);
		co2Data[date] = averageSeaLevel;

		previousMonth = month;
		valuesInMonth = 1;
		accumSeaLevel = seaLevel;
	}

	input2.close();

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
			if (year == 2017 && month > 9)
				continue;

			temperature = strtof(values.at(month).c_str(), 0);
			char date[8];
			sprintf(date, "%04d-%02d", year, month);
			temperatureData[date] = temperature;
		}

	}
	input.close();
	return temperatureData;
}

std::map<string, float> DataLoader::loadDailyArcticIceArea1978() {
	map<string, float> arcticData;

	ifstream input("data\\nsidc_NH_SH_nt_final_and_nrt.txt");
	if (!input)
		return arcticData;

	float area;
	for (string line; getline(input, line);) {
		if (line[0] == ' ' || line[0] == '#' || line[0] == '\t')
			continue;

		vector<string> values = split(line, ',');
		vector<string> date = split(values.at(0), ' ');

		area = strtof(values.at(7).c_str(), 0);
		arcticData[date.at(0)] = area;

	}
	input.close();
	return arcticData;
}

std::map<string, float> DataLoader::loadDailyAntarcticIceArea1978() {
	map<string, float> antarcticData;

	ifstream input("data\\nsidc_NH_SH_nt_final_and_nrt.txt");
	if (!input)
		return antarcticData;

	float area;
	for (string line; getline(input, line);) {
		if (line[0] == ' ' || line[0] == '#' || line[0] == '\t')
			continue;

		vector<string> values = split(line, ',');
		vector<string> date = split(values.at(0), ' ');

		area = strtof(values.at(9).c_str(), 0);
		antarcticData[date.at(0)] = area;

	}
	input.close();
	return antarcticData;
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

std::string DataLoader::loadCitations() {
	ifstream input("data\\citations.txt");
	string toReturn;

	for (string line; getline(input, line);) {
		toReturn += line + "\n";
	}

	return toReturn;
}