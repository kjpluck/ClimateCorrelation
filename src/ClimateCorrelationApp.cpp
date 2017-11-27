#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include <fstream>
#include <math.h>

#include "Resources.h"
#include "DataLoader.h"
#include "Lerp.h"

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
	map<string, float> dailyArcticAreaData1978;
	map<string, float> dailyAntarcticAreaData1978;

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

	struct Co2Segment {
		float height;
		Color color;
	};

	vector<Co2Segment> co2Cylinder;
	vector<BarrelSegment> co2Barrel;
	vector<BarrelSegment> temperatureBarrel;
	vector<BarrelSegment> seaLevelBarrel;
	vector<BarrelSegment> arcticSeaIceBarrel;
	vector<BarrelSegment> antarcticSeaIceBarrel;

	float barrelRadius;
	void drawBarrel(vector<BarrelSegment>, float x, float y, float z, float scale);
	void drawCo2Cylinder(vector<Co2Segment> cylinder, float x, float y, float z, float scale);
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
	dailyArcticAreaData1978 = DataLoader::loadDailyArcticIceArea1978();
	dailyAntarcticAreaData1978 = DataLoader::loadDailyAntarcticIceArea1978();
	mFont = Font("Arial", 32);

	barrelRadius = 10;
}



void ClimateCorrelationApp::update()
{
	float elapsedTime = getElapsedFrames() / 30.0f;

	float zoom = elapsedTime * 0.02f + 1.0f;
	mCamera.lookAt(vec3(0.0f, 10.0f, 30.0f * zoom), vec3(0.0f, elapsedTime/10.0, 0.0f));


	float time = elapsedTime * 2.0f + 1850.0f;
	
	float animationFraction =  (time - 1850.0f)/(2017.0f - 1850.0f);
	Color lineColor = Lerp::interpolate(Color(0, 0, 0.5f), Color(1, 1, 1), animationFraction);

	Date theDate = GetDayFromFractionalYear(time);
	float angleInBarrel = (theDate.month / 12.0f) * 2 * M_PI;

	char yearMonth[8];
	sprintf(yearMonth, "%04d-%02d", theDate.year, theDate.month);

	char yearMonthDay[11];
	sprintf(yearMonthDay, "%04d-%02d-%02d", theDate.year, theDate.month, theDate.day);

	bool yearMonthHasChanged = yearMonth != previousYearMonth;
	bool yearMonthDayHasChanged = yearMonthDay != previousYearMonthDay;

	if (theDate.year < 1958){
		co2CylinderHeight = annualCo2Data1850[to_string(theDate.year)];
		Co2Segment co2Segment = { co2CylinderHeight, lineColor };
		co2Cylinder.push_back(co2Segment);
	}
	else {
		if (yearMonthHasChanged) {
			float co2ppm = monthlyCo2Data1958[yearMonth];
			if (co2ppm > 0) {
				BarrelSegment barrelSegment = { angleInBarrel, co2ppm, 1, lineColor };
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

		if (monthlyTemperatureData1880.count(yearMonth) == 1) {
			BarrelSegment temperatureSegment = { angleInBarrel, monthlyTemperatureData1880[yearMonth], 1, lineColor };
			temperatureBarrel.push_back(temperatureSegment);
		}

		if (monthlySeaLevelData1880.count(yearMonth) == 1) {
			BarrelSegment seaLevelSegment = { angleInBarrel, monthlySeaLevelData1880[yearMonth], 1, lineColor };
			seaLevelBarrel.push_back(seaLevelSegment);
		}
	}

	angleInBarrel = (time - (int)time) * 2 * M_PI;
	if (yearMonthDayHasChanged && theDate.year >= 1978) {
		animationFraction = (time - 1978.0f) / (2017.0f - 1978.0f);
		lineColor = Lerp::interpolate(Color(0, 0, 0.5f), Color(1, 1, 1), animationFraction);

		if (dailyArcticAreaData1978.count(yearMonthDay) == 1) {
			BarrelSegment arcticAreaSegment = { angleInBarrel, dailyArcticAreaData1978[yearMonthDay], 1, lineColor };
			arcticSeaIceBarrel.push_back(arcticAreaSegment);
		}
		if (dailyAntarcticAreaData1978.count(yearMonthDay) == 1) {
			BarrelSegment antarcticAreaSegment = { angleInBarrel, dailyAntarcticAreaData1978[yearMonthDay], 1, lineColor };
			antarcticSeaIceBarrel.push_back(antarcticAreaSegment);
		}

	}
	char *txt;
	if (theDate.year * 100 + theDate.month > 201710)
		txt = "2017/11";
	else
		sprintf(txt, "%04d/%02d", theDate.year, theDate.month);

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

	drawCo2Cylinder(co2Cylinder, 0, co2OffSet, 0, co2Scale);

	float temperatureScale = 37.0;
	float temperatureOffSet = 0.8 * temperatureScale - 30.0f;

	float seaLevelScale = 0.2;
	float seaLevelOffSet = 184.5 * seaLevelScale - 15.0f;

	float arcticScale = 12.0;
	float arcticOffSet = 10.0f;

	drawBarrel(temperatureBarrel, 25, temperatureOffSet, 0, temperatureScale);
	drawBarrel(seaLevelBarrel, -25, seaLevelOffSet, 0, seaLevelScale);
	drawBarrel(co2Barrel, 0, co2OffSet, 0, co2Scale);
	drawBarrel(arcticSeaIceBarrel, -50.0, arcticOffSet, 0, arcticScale);
	drawBarrel(antarcticSeaIceBarrel, 50.0, arcticOffSet, 0, arcticScale);

	gl::setMatricesWindow(getWindowSize());
	gl::color(1, 1, 1);
	gl::draw(mTextTexture, vec2(10,10));
	
	//std::experimental::filesystem::v1::path p("frames/frame");
	//writeImage( ("frames/frame" + to_string(getElapsedFrames()) + ".png"), copyWindowSurface());
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

		gl::color(barrelSegment.color);
		gl::drawLine(vec3(x1 + x, y1 + y, z1 + z), vec3(x2 + x, y2 + y, z2 + z));
		lastBarrelSegment = barrelSegment;
	}

}

void ClimateCorrelationApp::drawCo2Cylinder(vector<Co2Segment> cylinder, float x, float y, float z, float scale) {

	Co2Segment lastCo2Segment;
	bool first = true;
	for (auto &cylinderSegment : cylinder) {
		if (first) {
			lastCo2Segment = Co2Segment{ 0, Color(0,0,0) };
			first = false;
		}

		cinder::geom::Cylinder co2Cylinder = cinder::geom::Cylinder();
		gl::color(cylinderSegment.color);
		co2Cylinder.set(vec3(x, lastCo2Segment.height * scale + y, z), vec3(x, cylinderSegment.height * scale + y, z));
		co2Cylinder.radius(10);
		gl::draw(co2Cylinder);

		lastCo2Segment = cylinderSegment;
	}
}


CINDER_APP(ClimateCorrelationApp, RendererGl, [&](App::Settings *settings) {
	settings->setWindowSize(960, 540); })
	