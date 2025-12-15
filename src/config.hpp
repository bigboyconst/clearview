#pragma once

#include <string>

struct Config {
	float minScale;
	float scrollSpeed;
	float dragFriction;
	float scaleFriction;

	Config(float minScale, float scrollSpeed, float dragFriction, float scaleFriction) 
	: minScale(minScale), scrollSpeed(scrollSpeed), dragFriction(dragFriction), scaleFriction(scaleFriction) {
		
	}
};

inline Config defaultConfig = Config(0.01f, 1.5f, 6.0f, 4.0f);