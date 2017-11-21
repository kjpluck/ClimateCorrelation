#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include <fstream>
#include <math.h>

#include "Resources.h"
#include "DataLoader.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class ClimateCorrelationApp : public App {
  public:
	void setup() override;
	void update() override;
	void draw() override;
private:
	map<string, float> annualCo2Data1850;
	map<string, float> monthlyCo2Data1958;
	map<string, float> monthlyTemperatureData1880;
	map<string, float> monthlySeaLevelData1880;
	int x;
	int y;
	float temperature;
	float seaLevel;
	gl::TextureRef		mTextTexture;
	Font				mFont;
	float co2CylinderHeight;
	string previousYearMonth;
	string previousYearMonthDay;

	struct BarrelSegment {
		float angle;
		float height;
		float lineWeight;
		Color color;
	};

	vector<BarrelSegment> co2Barrel;
	vector<BarrelSegment> temperatureBarrel;
	vector<BarrelSegment> seaLevelBarrel;
	vector<BarrelSegment> arcticSeaIceBarrel;
	vector<BarrelSegment> antarcticSeaIceBarrel;
};

struct Date {
	int year;
	int month;
	int day;
};

Date GetDayFromFractionalYear(float time) {
	int daysInMonth[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
	Date toReturn;
	toReturn.year = time;
	
	float fraction = time - toReturn.year;
	int dayOfYear = 365 * fraction + 1;
	for (int i = 0; i < 12; i++) {
		if (dayOfYear <= daysInMonth[i]) {
			toReturn.month = i + 1;
			toReturn.day = dayOfYear;
			return toReturn;
		}
		dayOfYear -= daysInMonth[i];
	}
	return toReturn;
}

void ClimateCorrelationApp::setup()
{
	Date foo = GetDayFromFractionalYear(2017.4959);
	annualCo2Data1850 = DataLoader::loadAnnualCo2Data1850();
	monthlyCo2Data1958 = DataLoader::loadMonthlyCo2Data1958();
	monthlyTemperatureData1880 = DataLoader::loadMonthlyTemperature1880();
	monthlySeaLevelData1880 = DataLoader::loadMonthlySeaLevelData1880();
	mFont = Font("Arial", 32);
}



void ClimateCorrelationApp::update()
{
	float time = getElapsedSeconds() * 2 + 1880;
	Date theDate = GetDayFromFractionalYear(time);
	float angleInBarrel = (time - theDate.year) * 2 * M_PI;

	char yearMonth[8];
	sprintf(yearMonth, "%04d-%02d", theDate.year, theDate.month);

	char yearMonthDay[11];
	sprintf(yearMonthDay, "%04d-%02d-%02d", theDate.year, theDate.month, theDate.day);

	bool yearMonthHasChanged = yearMonth != previousYearMonth;
	bool yearMonthDayHasChanged = yearMonthDay != previousYearMonthDay;

	if(theDate.year < 1958)
		co2CylinderHeight = annualCo2Data1850[to_string(theDate.year)];
	else {
		if (yearMonthHasChanged) {
			BarrelSegment barrelSegment = { angleInBarrel, monthlyCo2Data1958[yearMonth], 1, Color(1,1,1) };
			co2Barrel.push_back(barrelSegment);
		}
	}

	if (yearMonthHasChanged) {
		BarrelSegment temperatureSegment = { angleInBarrel, monthlyTemperatureData1880[yearMonth], 1, Color(1,1,1) };
		temperatureBarrel.push_back(temperatureSegment);

		BarrelSegment seaLevelSegment = { angleInBarrel, monthlySeaLevelData1880[yearMonth], 1, Color(1,1,1) };
		seaLevelBarrel.push_back(seaLevelSegment);
	}
		
	x = getWindowWidth() / 2;
	y = getWindowHeight();


	string txt = to_string(theDate.year) + ": " + to_string(co2CylinderHeight);
	TextBox tbox = TextBox().font(mFont).text(txt);
	mTextTexture = gl::Texture2d::create(tbox.render());

	if (yearMonthHasChanged)
		previousYearMonth = yearMonth;

	if (yearMonthDayHasChanged)
		previousYearMonthDay = yearMonthDay;
}

void ClimateCorrelationApp::draw()
{
	gl::clear(Color(0, 0, 0));
	gl::drawLine(vec2(x, y), vec2(x, y- co2CylinderHeight));
	gl::drawLine(vec2(x / 2, y), vec2(x / 2, y - (temperature * 100 + 300)));
	gl::drawLine(vec2(x*1.5, y), vec2(x*1.5, y - (seaLevel + 300)));
	gl::draw(mTextTexture, vec2(x, y - co2CylinderHeight));
}

CINDER_APP( ClimateCorrelationApp, RendererGl )
