#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <cstdlib>
#include <cstdint>

#include "gl_utils.hpp"

struct ScreenImage {
	int width;
	int height;
	std::uint8_t* pixels;
};

void flipY(int width, int height, std::uint8_t* pixels) {
	for (int y = 0; y < height / 2; y++) {
		for (int x = 0; x < width * 4; x++) {
			std::swap(
				pixels[y * width * 4 + x],
				pixels[(height - 1 - y) * width * 4 + x]
			);
		}
	}
}

ScreenImage capture() {
	Display* dsp = XOpenDisplay(nullptr);

	if (!dsp) {
		fprintf(stderr, "Could not open display!\n");
		exit(EXIT_FAILURE);
	}

	int screen = DefaultScreen(dsp);
	Window root = RootWindow(dsp, screen);

	int width = DisplayWidth(dsp, screen);
	int height = DisplayHeight(dsp, screen);

	XImage* img = XGetImage(
		dsp,
		root,
		0, 0,
		width, height,
		AllPlanes,
		ZPixmap
	);

	if (!img) {
		fprintf(stderr, "Could not get image!\n");
		exit(EXIT_FAILURE);
	}

	std::uint8_t* pixels = new std::uint8_t[width * height * 4];

	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			unsigned long pixel = XGetPixel(img, x, y);

			std::uint8_t r = (pixel & img->red_mask) >> 16;
			std::uint8_t g = (pixel & img->green_mask) >> 8;
			std::uint8_t b = (pixel & img->blue_mask);

			int i = 4 * (y * width + x);
			pixels[i + 0] = r;
			pixels[i + 1] = g;
			pixels[i + 2] = b;
			pixels[i + 4] = 0xff;
		}
	}

	flipY(width, height, pixels);

	XDestroyImage(img);
	XCloseDisplay(dsp);

	return ScreenImage {width, height, pixels};
}

int main() {
	if (!glfwInit()) {
		fprintf(stderr, "Failed to initialize GLFW!\n");
		exit(EXIT_FAILURE);
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

	GLFWwindow* window = glfwCreateWindow(1920, 1080, "Window", nullptr, nullptr);

	if (!window) {
		fprintf(stderr, "Failed to create GLFW window!\n");
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(window);

	if (!gladLoadGL()) {
		fprintf(stderr, "Failed to initialize GLAD!\n");
		glfwDestroyWindow(window);
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	printf("[INFO] OpenGL version: %s\n", glGetString(GL_VERSION));

	ScreenImage screen = capture();

	GLuint screenTex;
	glGenTextures(1, &screenTex);
	glBindTexture(GL_TEXTURE_2D, screenTex);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexImage2D(GL_TEXTURE_2D,
		0,
		GL_RGBA8,
		screen.width,
		screen.height,
		0,
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		screen.pixels
	);

	delete[] screen.pixels;

	float quad[] = {
		-1, -1, 0, 0,
		 1, -1, 1, 0,
		 1,  1, 1, 1,

		-1, -1, 0, 0,
		 1,  1, 1, 1,
		-1,  1, 0, 1
	};

	GLuint vao, vbo;
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);

	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);

	GLuint program = loadShader("../src/shaders/testVert.glsl", "../src/shaders/testFrag.glsl");
	glUseProgram(program);
	glUniform1i(glGetUniformLocation(program, "u_tex"), 0);

	while (!glfwWindowShouldClose(window)) {
		glClear(GL_COLOR_BUFFER_BIT);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, screenTex);

		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &vbo);
	glDeleteTextures(1, &screenTex);
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}