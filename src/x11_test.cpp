#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3native.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>

#include <sys/ipc.h>
#include <sys/shm.h>

#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <cmath>

#include "gl_utils.hpp"
#include "nav.hpp"
#include "config.hpp"

struct ShmCapture {
	Display* display;
	Window root;
	XImage* image;
	XShmSegmentInfo shmInfo;
	int width;
	int height;
	std::uint8_t* pixels;
};

ShmCapture initShmCapture()
{
    ShmCapture cap{};

    cap.display = XOpenDisplay(nullptr);

    if (!cap.display) {
    	fprintf(stderr, "[ERROR] Couldn't open display!\n");
    	exit(EXIT_FAILURE);
    }

    int screen = DefaultScreen(cap.display);
    cap.root = RootWindow(cap.display, screen);

    cap.width  = DisplayWidth(cap.display, screen);
    cap.height = DisplayHeight(cap.display, screen);

    cap.image = XShmCreateImage(
        cap.display,
        DefaultVisual(cap.display, screen),
        DefaultDepth(cap.display, screen),
        ZPixmap,
        nullptr,
        &cap.shmInfo,
        cap.width,
        cap.height
    );

    if (!cap.image) {
    	fprintf(stderr, "[ERROR] Failed to create image!\n");
    	exit(EXIT_FAILURE);
    }

    // Allocate shared memory
    cap.shmInfo.shmid = shmget(
        IPC_PRIVATE,
        cap.image->bytes_per_line * cap.image->height,
        IPC_CREAT | 0777
    );

    if (cap.shmInfo.shmid < 0){
    	fprintf(stderr, "[ERROR] Invalid shm id\n");
    	exit(EXIT_FAILURE);
    }

    cap.shmInfo.shmaddr = (char*)shmat(cap.shmInfo.shmid, nullptr, 0);
    cap.image->data = cap.shmInfo.shmaddr;
    cap.shmInfo.readOnly = False;

    // Attach shared memory to X server
    if (!XShmAttach(cap.display, &cap.shmInfo)) {
    	fprintf(stderr, "[ERROR] Failed to attach display to X server!\n");
    	exit(EXIT_FAILURE);
    }

    XSync(cap.display, False);

    return cap;
}

void destroyShmCapture(ShmCapture& cap) {
	XShmDetach(cap.display, &cap.shmInfo);
	XDestroyImage(cap.image);

	shmdt(cap.shmInfo.shmaddr);
	shmctl(cap.shmInfo.shmid, IPC_RMID, nullptr);

	XCloseDisplay(cap.display);
}

void capture(ShmCapture& cap) {
	XShmGetImage(cap.display,
		cap.root,
		cap.image,
		0, 0,
		AllPlanes
	);
}

const float INITIAL_FL_DELTA_RAD = 250.0f;
const float FL_DELTA_RADIUS_DECELERATION = 10.0;

struct Flashlight {
	bool isEnabled;
	float shadow;
	float radius;
	float deltaRadius;

	void update(float dt) {
		if (std::abs(deltaRadius) > 1.0f) {
			radius = std::max(0.0f, radius + deltaRadius * dt);
			deltaRadius -= deltaRadius * FL_DELTA_RADIUS_DECELERATION * dt;
		}

		if (isEnabled) {
			shadow = std::min(shadow + 6 * dt, 0.8f);
		}
		else {
			shadow = std::max(shadow - 6 * dt, 0.0f);
		}
	}
};

struct Context {
	Config cfg;
	Camera camera;
	Mouse mouse;
	int ssWidth, ssHeight;
	glm::vec2 windowSize;
	Flashlight fl;

	void reset() {
		camera.scale = 1.0f;
		camera.deltaScale = 0.0f;
		camera.position = glm::vec2(0.0f, 0.0f);
		camera.velocity = glm::vec2(0.0f, 0.0f);
	}
};

Context ctx = Context{
	/*cfg*/ defaultConfig,
	/*camera*/ Camera{},
	/*mouse*/ Mouse{},
	/*ssWidth, ssHeight*/ 0, 0,
	/*windowSize*/ glm::vec2(),
	/*fl*/ Flashlight{
		false,
		0.2f,
		200.0f,
		0.0f
	}
};

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
	(void)xoffset;
	int ctrlState = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL);
	if (ctrlState == GLFW_PRESS) {
		if (yoffset > 0 && ctx.fl.isEnabled) {
			ctx.fl.deltaRadius += INITIAL_FL_DELTA_RAD;
		}
		else if (yoffset < 0 && ctx.fl.isEnabled) {
			ctx.fl.deltaRadius -= INITIAL_FL_DELTA_RAD;
		}
	}
	// ctrl is not pressed
	else {
		if (yoffset > 0) {
			ctx.camera.deltaScale += ctx.cfg.scrollSpeed;
			ctx.camera.scalePivot = ctx.mouse.current;
		}
		else if (yoffset < 0) {
			ctx.camera.deltaScale -= ctx.cfg.scrollSpeed;
			ctx.camera.scalePivot = ctx.mouse.current;
		}
	}
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	(void)scancode;
	(void)mods;
	if (action == GLFW_PRESS) {
		if (key == GLFW_KEY_0) {
			ctx.reset();
		}
		else if (key == GLFW_KEY_Q || key == GLFW_KEY_ESCAPE) {
			glfwSetWindowShouldClose(window, GLFW_TRUE);
			exit(EXIT_SUCCESS);
		}
		else if (key == GLFW_KEY_F) {
			ctx.fl.isEnabled = !ctx.fl.isEnabled;
		}
	}
}

void updateUniforms(GLuint shader) {
	glUseProgram(shader);
	
	GLint _camPosLoc  = glGetUniformLocation(shader, "u_cameraPos");
	if (_camPosLoc != -1) {
		glUniform2f(_camPosLoc, ctx.camera.position.x, ctx.camera.position.y);
	}

	GLint _camScaleLoc = glGetUniformLocation(shader, "u_cameraScale");
	if (_camScaleLoc != -1) {
		glUniform1f(_camScaleLoc, ctx.camera.scale);
	}

	GLint _scSizeLoc = glGetUniformLocation(shader, "u_screenshotSize");
	if (_scSizeLoc != -1) {
		glUniform2f(_scSizeLoc, (float)ctx.ssWidth, (float)ctx.ssHeight);
	}

	GLint _winSizeLoc = glGetUniformLocation(shader, "u_windowSize");
	if (_winSizeLoc != -1) {
		glUniform2f(_winSizeLoc, ctx.windowSize.x, ctx.windowSize.y);
	}

	GLint _cursorPosLoc = glGetUniformLocation(shader, "u_cursorPos");
	if (_cursorPosLoc != -1) {
		glUniform2f(_cursorPosLoc, ctx.mouse.current.x, ctx.mouse.current.y);
	}

	GLint _flShadowLoc = glGetUniformLocation(shader, "u_flShadow");
	if (_flShadowLoc != -1) {
		glUniform1f(_flShadowLoc, ctx.fl.shadow);
	}

	GLint _flRadLoc = glGetUniformLocation(shader, "u_flRadius");
	if (_flRadLoc != -1) {
		glUniform1f(_flRadLoc, ctx.fl.radius);
	}
}

int main() {
	if (!glfwInit()) {
		fprintf(stderr, "[ERROR] Failed to initialize GLFW!\n");
		exit(EXIT_FAILURE);
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

	// Disable window header
	glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);

	GLFWmonitor* monitor = glfwGetPrimaryMonitor();

	if (!monitor) {
		fprintf(stderr, "[ERROR] Couldn't get primary monitor (how the fuck did you get into this situation?)!\n");
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	const GLFWvidmode* mode = glfwGetVideoMode(monitor);
	
	if (!mode) {
		fprintf(stderr, "[ERROR] Couldn't get video mode from monitor!\n");
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	float fps = (float)mode->refreshRate;

	GLFWwindow* window = glfwCreateWindow(
		mode->width, mode->height, 
		"Window", 
		nullptr, 
		nullptr
	);

	if (!window) {
		fprintf(stderr, "[ERROR] Failed to create GLFW window!\n");
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	Display* xdisplay = glfwGetX11Display();
	Window xwin = glfwGetX11Window(window);

	XSetWindowAttributes attrs{};
	attrs.override_redirect = True;

	XChangeWindowAttributes(xdisplay,
		xwin,
		CWOverrideRedirect,
		&attrs
	);

	XFlush(xdisplay);


	glfwMakeContextCurrent(window);
	glfwSetWindowPos(window, 0, 0);
	glfwSetWindowAttrib(window, GLFW_FLOATING, GLFW_TRUE);

	glfwSetScrollCallback(window, scrollCallback);
	glfwSetKeyCallback(window, keyCallback);

	if (!gladLoadGL()) {
		fprintf(stderr, "[ERROR] Failed to initialize GLAD!\n");
		glfwDestroyWindow(window);
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	printf("[INFO] OpenGL version: %s\n", glGetString(GL_VERSION));

	ShmCapture screen = initShmCapture();
	capture(screen);

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
		GL_BGRA,
		GL_UNSIGNED_BYTE,
		screen.image->data
	);

	int sw = screen.width;
	int sh = screen.height;

	ctx.ssWidth = sw; ctx.ssHeight = sh;

	float quad[] = {
		0,   0,     0, 0,
		sw,  0,     1, 0,
		sw, sh,     1, 1,

		0,   0,     0, 0,
		sw, sh,     1, 1,
		0,  sh,     0, 1
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

	GLuint program = loadShader(
		"../src/shaders/vert.glsl", 
		"../src/shaders/frag.glsl"
	);
	glUseProgram(program);
	glUniform1i(glGetUniformLocation(program, "u_tex"), 0);

	{
		double m_x, m_y;
		glfwGetCursorPos(window, &m_x, &m_y);
		glm::vec2 pos = glm::vec2((float)m_x, (float)m_y);
		ctx.mouse.current = pos;
		ctx.mouse.previous = pos;
	}
	
	ctx.fl.radius = 200.0f;
	ctx.fl.isEnabled = false;
	ctx.cfg = defaultConfig;

	float prevTime, currTime;

	float dt = 0.0f;

	while (!glfwWindowShouldClose(window)) {
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		prevTime = currTime;
		currTime = (float)glfwGetTime();
		dt = std::max(0.0f, currTime - prevTime);

		// get window size
		int w, h;
		glfwGetFramebufferSize(window, &w, &h);
		ctx.windowSize = glm::vec2((float)w, (float)h);

		// update mouse & camera position
		{
			double m_x, m_y;
			glfwGetCursorPos(window, &m_x, &m_y);
			ctx.mouse.current = glm::vec2((float)m_x, (float)m_y);
		}

		int lmbState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
		
		if (!ctx.mouse.dragging && lmbState == GLFW_PRESS) {
			ctx.mouse.previous = ctx.mouse.current;
			ctx.mouse.dragging = true;
			ctx.camera.velocity = glm::vec2(0.0f, 0.0f);
		}
		else if (lmbState == GLFW_RELEASE) {
			ctx.mouse.dragging = false;
		}

		if (ctx.mouse.dragging) {
			glm::vec2 delta = ctx.camera.world(ctx.mouse.previous) - ctx.camera.world(ctx.mouse.current);
			ctx.camera.position += delta;
			ctx.camera.velocity = delta * fps;
		}
		
		ctx.mouse.previous = ctx.mouse.current;

		// update camera with new mouse

		ctx.camera.update(ctx.cfg, dt, ctx.mouse, ctx.windowSize);

		// update flashlight
		ctx.fl.update(dt);

		// update the uniforms
		updateUniforms(program);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, screenTex);

		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	destroyShmCapture(screen);
	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &vbo);
	glDeleteTextures(1, &screenTex);
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}