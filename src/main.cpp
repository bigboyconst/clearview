#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "platform_types.hpp"

#include "gl_utils.hpp"

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

	Capture capture;
	if (!capture.capture()) {
		fprintf(stderr, "Failed to capture screen!\n");
		glfwDestroyWindow(window);
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	int w = capture.width();
	int h = capture.height();

	const uint8_t* drawData = capture.data();

	GLuint tex = createTexture(capture.width(), capture.height());
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	glTexImage2D(GL_TEXTURE_2D,
		0,
		GL_RGBA8,
		w,
		h,
		0,
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		drawData 
	);

	float vertices[] = {
		-1, -1,     0, 0, // bottom left
		 1, -1,     1, 0, // bottom right
		-1,  1,     0, 1, // top left
		 1,  1,     1, 1  // top right
	};

	int indices[] = {
		0, 1, 2,
		1, 2, 3
	};

	GLuint vao;
	GLuint vbo;
	GLuint ebo;
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glGenBuffers(1, &ebo);

	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	GLuint stride = 4 * sizeof(float);

	// aPos
	glVertexAttribPointer(0, 2, GL_FLOAT, false, stride, nullptr);
	glEnableVertexAttribArray(0);

	// aUV
	glVertexAttribPointer(1, 2, GL_FLOAT, false, stride, (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);

	GLuint program = loadShader("../src/shaders/testVert.glsl", "../src/shaders/testFrag.glsl");

	glUseProgram(program);

	glUniform1i(glGetUniformLocation(program, "u_tex"), 0);

	while (!glfwWindowShouldClose(window)) {
		glClear(GL_COLOR_BUFFER_BIT);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tex);

		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &vbo);
	glDeleteBuffers(1, &ebo);
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}