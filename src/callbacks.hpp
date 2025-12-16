#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "nav.hpp"

static void CB_framebuffer(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
}

static void CB_cursorPos(GLFWwindow* window, double xpos, double ypos) {
	
}

// void CB_key(GLFWwindow* window, int key, int scancode, int action, int mods) {
// 	// user pointer: camera
// 	Camera* cam = (Camera*)glfwGetWindowUserPointer(window);


// }