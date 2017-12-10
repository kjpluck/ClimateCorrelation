#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder\CinderGlm.h"
#include "cinder/Timeline.h"
#include "cinder/Easing.h"
#include <boost/date_time/gregorian/gregorian.hpp>
#include <fstream>
#include <math.h>
#include <iomanip> // setprecision
#include <sstream> // stringstream

#include "Resources.h"
#include "DataLoader.h"
#include "Lerp.h"
#include "DateUtils.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace boost::gregorian;



class ClimateCorrelationApp : public App {
  public:
	  void setup() override;
	void update() override;
	void draw() override;
	void drawCitations();
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
	string previousYearMonth;
	string previousYearMonthDay;
	date previousBoostDate;
	float mTitlePosition;
	int mYear;
	float mCurrentMaxCo2;
	Anim<float> mEaseCo2Title = 20.0f;
	Anim<float> mEaseSeaLevelTemperatureTitle = 20.0f;
	bool mFirstFrameOfSeaLevelAndTemperature = true;
	Anim<float> mEaseSeaIceTitle = 20.0f;
	bool mFirstFrameOfSeaIce = true;

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
	void drawSeaIceGraduations(float x, float y, float z, float minimum, float maximum, float scale);
	void drawTemperatureGraduations(float x, float y, float z, float maximum, float scale);
	void drawSeaLevelGraduations(float x, float y, float z, float maximum, float scale);
	void drawCo2Gradutations(float x, float y, float z, float scale);
	void setPixels(Surface * surface, int from, int to, Color8u color);
	void drawText(vec3 position, float angle, const std::string text, float = 1.0f, TextBox::Alignment = TextBox::CENTER, ColorA color = ColorA(1,1,1,1));
	void fadeTextInOut(vec3 position, float angle, const std::string text, float fadeInTime, float duration, float scale = 1.0f, TextBox::Alignment alignment = TextBox::CENTER, Color color = Color(1, 1, 1));
	std::string floatToStr(float value, int precision);
	float mCurrentMaxSeaLevel;
	float mCurrentMaxTemperature;
	float mCurrentArcticSeaIceMinimum;
	float mCurrentArcticSeaIceMaximum;
	float mCurrentAntarcticSeaIceMinimum;
	float mCurrentAntarcticSeaIceMaximum;
	float elapsedTime;
	string mCitations;

	Anim<float> afoo;
	TimelineRef mTimeline = Timeline::create();
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
	glLineWidth(3.0f);
	setFullScreen(true);
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
	mCitations = DataLoader::loadCitations();

	mFont = Font("Arial", 32);

	barrelRadius = 9;
	mCurrentMaxCo2 = 0.0;
	mCurrentMaxSeaLevel = -1000;
	mCurrentMaxTemperature = -1000;

	mCurrentAntarcticSeaIceMaximum = -1000;
	mCurrentAntarcticSeaIceMinimum = 1000;
	mCurrentArcticSeaIceMaximum = -1000;
	mCurrentArcticSeaIceMinimum = 1000;

	(*mTimeline).apply(&mEaseCo2Title, 0.0f, 1.5f, EaseOutBack(1.1f));
}



void ClimateCorrelationApp::update()
{
	(*mTimeline).step(1.0 / 30.0);
	elapsedTime = getElapsedFrames() / 30.0f;

	float zoom = elapsedTime * 0.02f + 1.0f;
	mCamera.lookAt(vec3(0.0f, 10.0f, 30.0f * zoom), vec3(0.0f, elapsedTime/10.0, 0.0f));

	mTitlePosition = elapsedTime/2.5  + 10;

	float time = elapsedTime * 2.0f + 1850.0f;
	
	float animationFraction =  (time - 1850.0f)/(2017.0f - 1850.0f);
	// 117,107,177 -> 239,237,245
	Color lineColor = Lerp::interpolate(Color(117.0/255.0, 107.0/255.0 , 177.0/255.0), Color(239.0/255.0, 237.0/255.0, 245.0/255.0), animationFraction);

	Date theDate = GetDayFromFractionalYear(time);

	date boostDate(theDate.year, theDate.month, theDate.day);

	float angleInBarrel = (theDate.month / 12.0f) * 2 * M_PI;

	char yearMonth[8];
	sprintf(yearMonth, "%04d-%02d", theDate.year, theDate.month);

	char yearMonthDay[11];
	sprintf(yearMonthDay, "%04d-%02d-%02d", theDate.year, theDate.month, theDate.day);

	bool yearMonthHasChanged = yearMonth != previousYearMonth;
	bool yearMonthDayHasChanged = yearMonthDay != previousYearMonthDay;

	float theCo2ppm = 0;

	if (theDate.year < 1958){
		float co2ppm = annualCo2Data1850[to_string(theDate.year)];
		theCo2ppm = annualCo2Data1850[to_string(theDate.year)];
		Co2Segment co2Segment = { co2ppm, lineColor };
		co2Cylinder.push_back(co2Segment);
	}
	else {
		if (yearMonthHasChanged) {
			float co2ppm = monthlyCo2Data1958[yearMonth];
			theCo2ppm = monthlyCo2Data1958[yearMonth];
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
	
	if (theCo2ppm > mCurrentMaxCo2)
		mCurrentMaxCo2 = theCo2ppm;


	lineColor = Lerp::interpolate(Color(49.0 / 255.0, 163.0 / 255.0, 84.0 / 255.0), Color(229.0 / 255.0, 245.0 / 255.0, 224.0 / 255.0), animationFraction);

	if (theDate.year >= 1880 && yearMonthHasChanged) {

		if (monthlyTemperatureData1880.count(yearMonth) == 1) {
			
			float theTemperature = monthlyTemperatureData1880[yearMonth];
			if (theTemperature > mCurrentMaxTemperature)
				mCurrentMaxTemperature = theTemperature;

			BarrelSegment temperatureSegment = { angleInBarrel, monthlyTemperatureData1880[yearMonth], 1, lineColor };
			temperatureBarrel.push_back(temperatureSegment);
		}

		if (monthlySeaLevelData1880.count(yearMonth) == 1) {

			float theSeaLevel = monthlySeaLevelData1880[yearMonth];
			if (theSeaLevel > mCurrentMaxSeaLevel)
				mCurrentMaxSeaLevel = theSeaLevel;
			
			BarrelSegment seaLevelSegment = { angleInBarrel, monthlySeaLevelData1880[yearMonth], 1, lineColor };
			seaLevelBarrel.push_back(seaLevelSegment);
		}
	}

	int daysOfIce = 0;
	if (yearMonthDayHasChanged && theDate.year >= 1978) {
		animationFraction = (time - 1978.0f) / (2017.0f - 1978.0f);
		// 49,130,189 -> 222,235,247
		lineColor = Lerp::interpolate(Color(49.0 / 255.0, 130.0 / 255.0, 189.0 / 255.0), Color(222.0 / 255.0, 235.0 / 255.0, 247.0 / 255.0), animationFraction);
		
		date currentBoostDate(from_simple_string(previousYearMonthDay));


		while (currentBoostDate <= boostDate) {
			daysOfIce++;
			
			int dayOfYear = currentBoostDate.day_of_year();
			angleInBarrel = (dayOfYear / 365.0f) * 2 * M_PI;
			
			string currentYearMonthDay = DateUtils::formatDate(currentBoostDate, "%Y-%m-%d");

			if (dailyArcticAreaData1978.count(currentYearMonthDay) == 1) {

				float theArea = dailyArcticAreaData1978[currentYearMonthDay];
				if (theArea > mCurrentArcticSeaIceMaximum)
					mCurrentArcticSeaIceMaximum = theArea;
				if (theArea < mCurrentArcticSeaIceMinimum)
					mCurrentArcticSeaIceMinimum = theArea;

				BarrelSegment arcticAreaSegment = { angleInBarrel, dailyArcticAreaData1978[currentYearMonthDay], 1, lineColor };
				arcticSeaIceBarrel.push_back(arcticAreaSegment);
			}
			if (dailyAntarcticAreaData1978.count(currentYearMonthDay) == 1) {

				float theArea = dailyAntarcticAreaData1978[currentYearMonthDay];
				if (theArea > mCurrentAntarcticSeaIceMaximum)
					mCurrentAntarcticSeaIceMaximum = theArea;
				if (theArea < mCurrentAntarcticSeaIceMinimum)
					mCurrentAntarcticSeaIceMinimum = theArea;

				BarrelSegment antarcticAreaSegment = { angleInBarrel, dailyAntarcticAreaData1978[currentYearMonthDay], 1, lineColor };
				antarcticSeaIceBarrel.push_back(antarcticAreaSegment);
			}

			currentBoostDate += days(1);
		}

	}
	
	if (theDate.year * 100 + theDate.month > 201710)
		mYear = 2017;
	else
		mYear = theDate.year;
	
	string selfPromo = "@kevpluck ";// +to_string(daysOfIce) + " "; // +floatToStr(elapsedTime, 2);
	TextBox tbox = TextBox().font(mFont).text(selfPromo).size(400,40);
	
	mTextTexture = gl::Texture2d::create(tbox.render());

	if (yearMonthHasChanged)
		previousYearMonth = yearMonth;

	if (yearMonthDayHasChanged)
	{
		previousYearMonthDay = yearMonthDay;
		previousBoostDate = boostDate;
	}
}

void ClimateCorrelationApp::draw()
{
	gl::color(1, 1, 1);
	gl::clear(Color(0, 0.0f, 0.15f));
	gl::setMatrices(mCamera);

	float co2Scale = 0.4;
	float co2OffSet = -315 * co2Scale;

	drawCo2Cylinder(co2Cylinder, 0, co2OffSet, 0, co2Scale);
	drawCo2Gradutations(0, co2OffSet, 0, co2Scale);

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


	drawText(vec3(0, mTitlePosition + mEaseCo2Title, -0.01), 0, "Carbon Dioxide\nConcentration (ppm)\n" + std::to_string(mYear) );
	
	if (temperatureBarrel.size() > 0) {

		if (mFirstFrameOfSeaLevelAndTemperature)
		{
			(*mTimeline).apply(&mEaseSeaLevelTemperatureTitle, 0.0f, 1.5f, EaseOutBack(1.1f));
			mFirstFrameOfSeaLevelAndTemperature = false;
		}
		drawText(vec3(25, mTitlePosition + mEaseSeaLevelTemperatureTitle, -0.01), 0, "Global Temperature\n1951-1980 mean\n(deg. C)");
		drawTemperatureGraduations(25, temperatureOffSet, 0, mCurrentMaxTemperature, temperatureScale);
	}

	if (seaLevelBarrel.size() > 0) {
		drawText(vec3(-25, mTitlePosition + mEaseSeaLevelTemperatureTitle, -0.01), 0, "Global Sea Level\n(mm)");
		drawSeaLevelGraduations(-25, seaLevelOffSet, 0, mCurrentMaxSeaLevel, seaLevelScale);
	}
	
	if (arcticSeaIceBarrel.size() > 0) {
		if (mFirstFrameOfSeaIce) {
			(*mTimeline).apply(&mEaseSeaIceTitle, 0.0f, 1.5f, EaseOutBack(1.1f));
			mFirstFrameOfSeaIce = false;
		}
		drawText(vec3(-50, mTitlePosition + mEaseSeaIceTitle, -0.01), 0, "Arctic Sea Ice\n1981-2010 mean\n(x10,000 sq. km)");
		drawSeaIceGraduations(-50, arcticOffSet, 0, mCurrentArcticSeaIceMinimum, mCurrentArcticSeaIceMaximum, arcticScale);
	}
	
	if (antarcticSeaIceBarrel.size() > 0) {
		drawText(vec3(50, mTitlePosition + mEaseSeaIceTitle, -0.01), 0, "Antarctic Sea Ice\n1981-2010 mean\n(x10,000 sq. km)");
		drawSeaIceGraduations(51, arcticOffSet, 0, mCurrentAntarcticSeaIceMinimum, mCurrentAntarcticSeaIceMaximum, arcticScale);
	}
	
	if (co2Barrel.size() > 0) {
		fadeTextInOut(vec3(0, 2.5, 10), 0, "Direct\nMeasurement\nMauna Loa", 54.0f, 5.0f, 0.7f, TextBox::CENTER, Color(0,0,0.15f));
	}


	gl::setMatricesWindow(getWindowSize());
	gl::color(1, 1, 1);
	gl::draw(mTextTexture, vec2(10, getWindowSize().y - 35));

	if (elapsedTime > 97)
		drawCitations();

	writeImage(("frames/frame" + to_string(getElapsedFrames()) + ".png"), copyWindowSurface());

	if (elapsedTime > 103)
		quit();
}

void ClimateCorrelationApp::fadeTextInOut(vec3 position, float angle, const std::string text, float fadeInTime, float duration, float scale, TextBox::Alignment alignment, Color color) {
	if (elapsedTime < fadeInTime || elapsedTime > fadeInTime + duration)
		return;

	float endTime = fadeInTime + duration;
	float fade = 1.0f;
	
	if (elapsedTime - fadeInTime < 1.0f)
		fade = (elapsedTime - fadeInTime) / 1.0;
	if (elapsedTime > endTime - 1.0f)
		fade = -(elapsedTime - endTime) / 1.0f;

	ColorA fadeColor = ColorA(color, fade);
	drawText(position, angle, text, scale, alignment, fadeColor);
}

void ClimateCorrelationApp::drawCitations() {
	TextBox tbox = TextBox().font(mFont).text(mCitations).size(1720, 880).alignment(TextBox::CENTER);
	tbox.setColor(Color(1, 1, 1));
	tbox.setBackgroundColor(ColorA(0, 0, 0, 0.7));


	gl::Texture2dRef textTexture = gl::Texture2d::create(tbox.render());
	gl::draw(textTexture, vec2(100, 100));
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
	float iceCoreCo2Max = 315.3;

	if (mYear < 1958)
		iceCoreCo2Max = mCurrentMaxCo2;

	Surface co2TimeGradient(1, iceCoreCo2Max, false);

	bool first = true;
	for (auto &cylinderSegment : cylinder) {
		if (first) {
			lastCo2Segment = Co2Segment{ 0, Color(117.0 / 255.0, 107.0 / 255.0 , 177.0 / 255.0) };
			setPixels(&co2TimeGradient, 0, lastCo2Segment.height, lastCo2Segment.color);
			first = false;
		}

		setPixels(&co2TimeGradient, lastCo2Segment.height, cylinderSegment.height, cylinderSegment.color);

		lastCo2Segment = cylinderSegment;
	}

	auto co2TimeTexture = ci::gl::Texture::create(co2TimeGradient);
	co2TimeTexture->bind();

	auto shader = gl::ShaderDef().texture();
	gl::GlslProgRef glsl = gl::getStockShader(shader);
	
	cinder::geom::Cylinder co2Cylinder = cinder::geom::Cylinder();
	co2Cylinder.set(vec3(x, 0 + y, z), vec3(x, iceCoreCo2Max * scale + y, z));
	co2Cylinder.radius(barrelRadius);

	gl::BatchRef cylinderBatch = gl::Batch::create(co2Cylinder, glsl);
	cylinderBatch->draw();
	
	float fadeOut = (30.0f - elapsedTime) / 30.0f;

	if (fadeOut < 0.0f) 
		return;

	gl::pushMatrices();
	gl::translate(vec3(x, iceCoreCo2Max * scale + y + 0.1, z - 2.0f));
	gl::rotate(-M_PI / 2, vec3(1, 0, 0));
	fadeTextInOut(vec3(0,0,0), 0, "Ice Core\nObservations\nAntarctica", 6.0f, 5.0f, 1, TextBox::CENTER, Color(0, 0, 0.15f));
	gl::popMatrices();
}

void ClimateCorrelationApp::drawSeaIceGraduations(float x, float y, float z, float minimum, float maximum, float scale) {

	int min = minimum * 100;
	int toNearest50 = min % 50;
	min -= toNearest50;

	int max = maximum * 100;

	for (int graduation = min; graduation < max; graduation += 50) {
		char value[10];
		sprintf(value, "%4d", graduation);
		drawText(vec3(x + 12.0f, graduation * (scale / 100.0) + y, z), 0, value, 0.7f, TextBox::RIGHT);
	}
}

void ClimateCorrelationApp::drawTemperatureGraduations(float x, float y, float z, float maximum, float scale) {

	for (float graduation = -0.8; graduation < maximum; graduation += 0.1) {
		char value[10];
		sprintf(value, "%.1f", graduation);
		drawText(vec3(x + 12.0f, graduation * scale + y, z), 0, value, 0.7f, TextBox::RIGHT);
	}
}

void ClimateCorrelationApp::drawSeaLevelGraduations(float x, float y, float z, float maximum, float scale) {
	
	for (int graduation = 0; graduation < (maximum + 180); graduation += 20) {
		char value[10];
		sprintf(value, "%4d", graduation);
		drawText(vec3(x + 12.0f, (graduation - 180) * scale + y, z), 0, value, 0.7f, TextBox::RIGHT);
	}
}

void ClimateCorrelationApp::drawCo2Gradutations(float x, float y, float z, float scale) {

	for (int co2Graduation = 250; co2Graduation < mCurrentMaxCo2; co2Graduation += 10) {
		drawText(vec3(x + 12.0f, co2Graduation * scale + y, z), 0, std::to_string(co2Graduation), 0.7f);
	}
}

void ClimateCorrelationApp::setPixels(Surface *surface, int from, int to, Color8u color) {
	for (int i = from; i <= to; i++) {
		surface->setPixel(vec2(0, i), color);
	}
}

void ClimateCorrelationApp::drawText(vec3 position, float angle, const std::string text, float scale, TextBox::Alignment alignment, ColorA color)
{
	TextBox tbox = TextBox().alignment(alignment).font(mFont).text(text).color(color);
	gl::TextureRef textTexture = gl::Texture2d::create(tbox.render());
	textTexture->bind();
	vec2 textBoxSize = tbox.measure() * 0.1f * scale;


	gl::pushMatrices();
	gl::rotate(angleAxis(angle,vec3(0,1,0)));
	gl::translate(position);
	gl::translate(vec3(-textBoxSize.x / 2.0f, 0, 0));
	gl::scale(vec2(1, -1));
	auto shader = gl::ShaderDef().texture();// .lambert();
	gl::GlslProgRef glsl = gl::getStockShader(shader);
	auto plane = geom::Rect().rect(Rectf(0, 0, textBoxSize.x, textBoxSize.y));
	gl::BatchRef planeBatch = gl::Batch::create(plane, glsl);
	planeBatch->draw();
	gl::popMatrices();
}

std::string ClimateCorrelationApp::floatToStr(float value, int precision) {
	stringstream stream;
	stream << fixed << setprecision(precision) << value;
	return stream.str();
}



CINDER_APP(ClimateCorrelationApp, RendererGl, [&](App::Settings *settings) {
	settings->setWindowSize(1920, 1080); })
	