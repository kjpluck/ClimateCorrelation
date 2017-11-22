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
	CameraPersp		mCamera;
private:
	map<string, float> annualCo2Data1850;
	map<string, float> monthlyCo2Data1958;
	map<string, float> monthlyTemperatureData1880;
	map<string, float> monthlySeaLevelData1880;
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

	float barrelRadius;
	void drawBarrel(vector<BarrelSegment>, float x, float y, float z, float scale);
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
	gl::enableDepthRead();
	gl::enableDepthWrite();


	mCamera.setPerspective(50.0f, getWindowAspectRatio(), 0.1f, 1000.0f);
	//mCamera.lookAt(vec3(0, 15, 30), vec3(0, 0, 0));

	Date foo = GetDayFromFractionalYear(2017.4959);
	annualCo2Data1850 = DataLoader::loadAnnualCo2Data1850();
	monthlyCo2Data1958 = DataLoader::loadMonthlyCo2Data1958();
	monthlyTemperatureData1880 = DataLoader::loadMonthlyTemperature1880();
	monthlySeaLevelData1880 = DataLoader::loadMonthlySeaLevelData1880();
	mFont = Font("Arial", 32);

	barrelRadius = 10;
}



void ClimateCorrelationApp::update()
{
	float elapsedTime = getElapsedFrames() / 30.0f;

	float zoom = elapsedTime * 0.02f + 1.0f;
	mCamera.lookAt(vec3(0.0f, 10.0f, 30.0f * zoom), vec3(0.0f, elapsedTime/10.0, 0.0f));


	float time = elapsedTime * 2 + 1850;
	Date theDate = GetDayFromFractionalYear(time);
	float angleInBarrel = (theDate.month / 12.0f) * 2 * M_PI;

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
			float co2ppm = monthlyCo2Data1958[yearMonth];
			if (co2ppm > 0) {
				BarrelSegment barrelSegment = { angleInBarrel, co2ppm, 1, Color(1,1,1) };
				co2Barrel.push_back(barrelSegment);
			}
			else {
				// Missing data in CO2 readings are set to -99
				// so copy the last CO2 reading and push onto end of vector
				if (co2Barrel.size() > 0) {
					BarrelSegment lastBarrelSegment = co2Barrel.back();

					BarrelSegment copyOfLast;
					copyOfLast.height = lastBarrelSegment.height;
					copyOfLast.angle = angleInBarrel;
					copyOfLast.lineWeight = lastBarrelSegment.lineWeight;
					copyOfLast.color = lastBarrelSegment.color;

					co2Barrel.push_back(copyOfLast);
				}
			}

		}
	}
	
	if (theDate.year >= 1880 && yearMonthHasChanged) {
		BarrelSegment temperatureSegment = { angleInBarrel, monthlyTemperatureData1880[yearMonth], 1, Color(1,1,1) };
		temperatureBarrel.push_back(temperatureSegment);

		BarrelSegment seaLevelSegment = { angleInBarrel, monthlySeaLevelData1880[yearMonth], 1, Color(1,1,1) };
		seaLevelBarrel.push_back(seaLevelSegment);
	}
		


	string txt = to_string(theDate.year) + "/" + to_string(theDate.month);
	TextBox tbox = TextBox().font(mFont).text(txt);
	mTextTexture = gl::Texture2d::create(tbox.render());

	if (yearMonthHasChanged)
		previousYearMonth = yearMonth;

	if (yearMonthDayHasChanged)
		previousYearMonthDay = yearMonthDay;
}

void ClimateCorrelationApp::draw()
{
	gl::color(1, 1, 1);
	gl::clear(Color(0, 0.0f, 0.15f));
	gl::setMatrices(mCamera);

	float co2Scale = 0.4;
	float co2OffSet = -315 * co2Scale;

	cinder::geom::Cylinder co2Cylinder = cinder::geom::Cylinder();
	co2Cylinder.set(vec3(0, co2OffSet, 0), vec3(0, co2CylinderHeight * co2Scale + co2OffSet, 0));
	co2Cylinder.radius(10);

	gl::draw(co2Cylinder);

	float temperatureScale = 37.0;
	float temperatureOffSet = 0.8 * temperatureScale - 30.0f;

	float seaLevelScale = 0.2;
	float seaLevelOffSet = 184.5 * seaLevelScale - 15.0f;

	drawBarrel(temperatureBarrel, 25, temperatureOffSet, 0, temperatureScale);
	drawBarrel(seaLevelBarrel, -25, seaLevelOffSet, 0, seaLevelScale);
	drawBarrel(co2Barrel, 0, co2OffSet, 0, co2Scale);

	gl::setMatricesWindow(getWindowSize());
	gl::draw(mTextTexture, vec2(10,10));
}

void ClimateCorrelationApp::drawBarrel(vector<BarrelSegment> barrel, float x, float y, float z, float scale) {

	BarrelSegment lastBarrelSegment;
	bool first = true;
	float rotationAngle = getElapsedFrames() * 0.005;
	for (auto &barrelSegment : barrel) {
		if (first) {
			lastBarrelSegment = barrelSegment;
			first = false;
			continue;
		}
		float x1 = cos(lastBarrelSegment.angle + rotationAngle) * barrelRadius;
		float z1 = sin(lastBarrelSegment.angle + rotationAngle) * barrelRadius;
		float y1 = lastBarrelSegment.height * scale;
		float x2 = cos(barrelSegment.angle + rotationAngle) * barrelRadius;
		float z2 = sin(barrelSegment.angle + rotationAngle) * barrelRadius;
		float y2 = barrelSegment.height * scale;

		gl::drawLine(vec3(x1 + x, y1 + y, z1 + z), vec3(x2 + x, y2 + y, z2 + z));
		lastBarrelSegment = barrelSegment;
	}
}

CINDER_APP(ClimateCorrelationApp, RendererGl, [&](App::Settings *settings) {
	settings->setWindowSize(1920, 1080); })
