#include "capture_x11.hpp"

#ifdef CAPTURE_X11

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <cstring>

struct X11ScreenCapture::Impl {
	Display* display = nullptr;
	Window root = 0;
	XImage* image = nullptr;
	int screen = 0;
	int w = 0;
	int h = 0;
};

X11ScreenCapture::X11ScreenCapture() {
	impl = new Impl();
	impl->display = XOpenDisplay(nullptr);

	if (!impl->display) {
		return;
	}

	impl->screen = DefaultScreen(impl->display);
	impl->root = RootWindow(impl->display, impl->screen);

	impl->w = DisplayWidth(impl->display, impl->screen);
	impl->h = DisplayHeight(impl->display, impl->screen);
}

X11ScreenCapture::~X11ScreenCapture() {
	if (impl->image) {
		XDestroyImage(impl->image);
	}
	if (impl->display) {
		XCloseDisplay(impl->display);
	}

	delete impl;
}

bool X11ScreenCapture::isValid() const {
	return impl->display != nullptr;
}

int X11ScreenCapture::width() const {
	return impl->w;
}

int X11ScreenCapture::height() const {
	return impl->h;
}

bool X11ScreenCapture::capture() {
	if (!isValid()) {
		return false;
	}

	if (impl->image) {
		XDestroyImage(impl->image);
		impl->image = nullptr;
	}

	impl->image = XGetImage(impl->display, 
		impl->root,
		0, 0,
		impl->w, impl->h,
		AllPlanes,
		ZPixmap
	);

	return impl->image != nullptr;
}

const uint8_t* X11ScreenCapture::data() const {
	if (!impl->image) {
		return nullptr;
	}
	return reinterpret_cast<const uint8_t*>(impl->image->data);
}
#endif