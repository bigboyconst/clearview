#pragma once

#include <cstdint>

class X11ScreenCapture {
public:
	X11ScreenCapture();
	~X11ScreenCapture();

	bool isValid() const;

	int width() const;
	int height() const;

	bool capture();

	const uint8_t* data() const;

private:
	struct Impl;
	Impl* impl;
};