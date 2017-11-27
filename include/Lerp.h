#pragma once

#include "cinder\app\App.h"
class Lerp
{
public:
	Lerp();
	~Lerp();
	static cinder::Color interpolate(cinder::Color a, cinder::Color b, float t);
};

