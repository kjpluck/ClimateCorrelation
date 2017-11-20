#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include <fstream>

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
	float co2;
	float temperature;
	float seaLevel;
	gl::TextureRef		mTextTexture;
	Font				mFont;
};

void ClimateCorrelationApp::setup()
{
	annualCo2Data1850 = DataLoader::loadAnnualCo2Data1850();
	monthlyCo2Data1958 = DataLoader::loadMonthlyCo2Data1958();
	monthlyTemperatureData1880 = DataLoader::loadMonthlyTemperature1880();
	monthlySeaLevelData1880 = DataLoader::loadMonthlySeaLevelData1880();
	mFont = Font("Arial", 32);
}


void ClimateCorrelationApp::update()
{
	float time = getElapsedSeconds() * 2 + 1880;
	int year = time;
	int month = (time - (int)time) * 12 + 1;

	char date[8];
	sprintf(date, "%04d-%02d", year, month);

	if(year < 1958)
		co2 = annualCo2Data1850[to_string(year)];
	else {
		co2 = monthlyCo2Data1958[date];
	}

	temperature = monthlyTemperatureData1880[date];
	seaLevel = monthlySeaLevelData1880[date];
		
	x = getWindowWidth() / 2;
	y = getWindowHeight();


	string txt = to_string(year) + ": " + to_string(co2);
	TextBox tbox = TextBox().font(mFont).text(txt);
	mTextTexture = gl::Texture2d::create(tbox.render());
}

void ClimateCorrelationApp::draw()
{
	gl::clear(Color(0, 0, 0));
	gl::drawLine(vec2(x, y), vec2(x, y-co2));
	gl::drawLine(vec2(x / 2, y), vec2(x / 2, y - (temperature * 100 + 300)));
	gl::drawLine(vec2(x*1.5, y), vec2(x*1.5, y - (seaLevel + 300)));
	gl::draw(mTextTexture, vec2(x, y - co2));
}

CINDER_APP( ClimateCorrelationApp, RendererGl )
