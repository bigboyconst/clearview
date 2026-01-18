cmake_minimum_required(VERSION 3.16)
project(clearview LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)

include(FetchContent)

FetchContent_Declare(
	glfw
	GIT_REPOSITORY https://github.com/glfw/glfw.git
	GIT_TAG 	   master
)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
	glm
	GIT_REPOSITORY https://github.com/g-truc/glm.git
	GIT_TAG        master
)
FetchContent_MakeAvailable(glm)

if (WIN32)
	set(CLEARVIEW_BINARY "clearview_win")
	# Windows GDI Build
	add_executable(clearview_win
		src/gdi_test.cpp
		thirdparty/glad/src/glad.c
	)
elseif (UNIX AND NOT APPLE)
	# TODO: Move find_package up here to create wayland binary
	set(CLEARVIEW_BINARY "clearview_x11")
	# X11 Build
	add_executable(clearview_x11
		src/x11_test.cpp
		thirdparty/glad/src/glad.c
	)
elseif (APPLE)
	message(FATAL_ERROR "MacOS is not yet supported")
endif()

target_include_directories(${CLEARVIEW_BINARY} PRIVATE 
	thirdparty
	thirdparty/glad/include
	${GLFW_SOURCE_DIR}/include
	${GLM_SOURCE_DIR}
)

target_link_libraries(${CLEARVIEW_BINARY} PRIVATE
	OpenGL::GL
	glfw
	glm::glm
)

# Platform-specific libraries

# Windows (GDI)
if (WIN32)
	target_compile_definitions(${CLEARVIEW_BINARY} PRIVATE CAPTURE_WIN_DXGI)
	target_link_libraries(${CLEARVIEW_BINARY} PRIVATE gdi32 user32)
endif()

# Mac (CoreGraphics)
if (APPLE)
	target_compile_definitions(${CLEARVIEW_BINARY} PRIVATE CAPTURE_MAC)
	target_link_libraries(${CLEARVIEW_BINARY} PRIVATE
		"-framework Cocoa"
		"-framework CoreGraphics"
		"-framework QuartzCore"
	)
endif()

# Linux (X11 / Wayland)
if (UNIX AND NOT APPLE)
	find_package(X11)
	find_package(Wayland)

	if (X11_FOUND)
		message(STATUS "X11 found: enabling X11 screen capture")
		target_compile_definitions(${CLEARVIEW_BINARY} PRIVATE CAPTURE_X11)
		target_link_libraries(${CLEARVIEW_BINARY} PRIVATE X11::X11 X11::Xext)
	elseif(Wayland_FOUND)
		message(STATUS "Wayland found: enabling Wayland screen capture")
		target_compile_definitions(${CLEARVIEW_BINARY} PRIVATE Wayland::Wayland)
	else()
		message(FATAL_ERROR
			"Neither X11 nor Wayland development libraries were found.\n"
			"Install libx11-dev or libwayland-dev with your package manager."
		)
	endif()
endif()

# Platform-specific configuration

if (MSVC)
	target_compile_options(${CLEARVIEW_BINARY} PRIVATE /W4)
else()
	target_compile_options(${CLEARVIEW_BINARY} PRIVATE -Wall -Wextra)
endif()

if (APPLE)
	target_compile_definitions(${CLEARVIEW_BINARY} PRIVATE GL_SILENCE_DEPRECATION)
endif()