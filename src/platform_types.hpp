#pragma once


#ifdef CAPTURE_WIN_DXGI
	#include "capture/capture_win_dxgi.hpp"
	
	using Image = XImage;
	using Capture = DXGIScreenCapture;
#elifdef CAPTURE_MAC
	#include "capture/capture_mac.hpp"
	

	using Capture = MacScreenCapture;
#elifdef CAPTURE_X11
	#include "capture/capture_x11.hpp"
	

	using Capture = X11ScreenCapture;
#elifdef CAPTURE_WAYLAND
	#include "capture/capture_wayland.hpp"
	

	using Capture = WaylandScreenCapture;
#endif