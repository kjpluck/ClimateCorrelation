#include "Lerp.h"
#include "cinder\app\App.h"

Lerp::Lerp() {}
Lerp::~Lerp() {}

cinder::Color Lerp::interpolate(cinder::Color a, cinder::Color b, float t)
{
	glm::vec3 ca = ci::rgbToHsv(a);
	glm::vec3 cb = ci::rgbToHsv(b);
	glm::vec3 final;

	final.x = ca.x * (1 - t) + cb.x*t;
	final.y = ca.y * (1 - t) + cb.y*t;
	final.z = ca.z * (1 - t) + cb.z*t;

	return ci::hsvToRgb(final);
}
