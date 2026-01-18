#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <Windows.h>

#include "gl_utils.hpp"
#include "nav.hpp"
#include "config.hpp"

#include "common.hpp"

template<typename T>
using vector = std::vector<T>;

class ScreenCaptureGDI {
private:
    HDC screenDC = nullptr;
    HDC memDC = nullptr;
    HBITMAP bitmap = nullptr;
    HBITMAP oldBitmap = nullptr;

public:
    GLuint glTexture = 0;
    int width = 0;
    int height = 0;
    vector<uint8_t> pixels;
    bool init() {
        width = GetSystemMetrics(SM_CXSCREEN);
        height = GetSystemMetrics(SM_CYSCREEN);

        screenDC = GetDC(nullptr);
        if (!screenDC) {
            fprintf(stderr, "Failed to get screen DC\n");
            return false;
        }

        memDC = CreateCompatibleDC(screenDC);
        if (!memDC) {
            fprintf(stderr, "Failed to create compatible DC\n");
            return false;
        }

        bitmap = CreateCompatibleBitmap(screenDC, width, height);
        if (!bitmap) {
            fprintf(stderr, "Failed to create bitmap\n");
            return false;
        }

        oldBitmap = (HBITMAP)SelectObject(memDC, bitmap);

        pixels.resize(width * height * 4);

        return true;
    }

    bool initGL() {
        glGenTextures(1, &glTexture);
        glBindTexture(GL_TEXTURE_2D, glTexture);

        glTexImage2D(GL_TEXTURE_2D,
            0,
            GL_RGBA,
            width,
            height,
            0,
            GL_BGRA,
            GL_UNSIGNED_BYTE,
            nullptr
        );

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glBindTexture(GL_TEXTURE_2D, 0);
        return true;
    }

    bool capture() {
        if (!BitBlt(memDC, 0, 0, width, height, screenDC, 0, 0, SRCCOPY)) {
            fprintf(stderr, "BitBlt failed!\n");
            return false;
        }

        BITMAPINFO bmi = {};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = width;
        bmi.bmiHeader.biHeight = -height;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        if (!GetDIBits(memDC, bitmap, 0, height, pixels.data(), &bmi, DIB_RGB_COLORS)) {
            fprintf(stderr, "GetDIBits failed!\n");
            return false;
        }

        return true;
    }

    bool updateTexture() {
        glBindTexture(GL_TEXTURE_2D, glTexture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_BGRA, GL_UNSIGNED_BYTE, pixels.data());
        glBindTexture(GL_TEXTURE_2D, 0);
        return true;
    }

    void cleanup() {
        if (memDC) {
            if (oldBitmap) SelectObject(memDC, oldBitmap);
            DeleteDC(memDC);
        }
        if (bitmap) DeleteObject(bitmap);
        if (screenDC) ReleaseDC(nullptr, screenDC);
        if (glTexture) glDeleteTextures(1, &glTexture);
    }
};

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

    printf("[INFO] OpenGL Version: %s\n", glGetString(GL_VERSION));

    ScreenCaptureGDI screen;
    if (!screen.init()) return 1;
    if (!screen.initGL()) return 1;
    if (!screen.capture()) return 1;
    if (!screen.updateTexture()) return 1;

    GLuint screenTex = screen.glTexture;

    int sw = screen.width;
    int sh = screen.height;

    ctx.ssWidth = sw; ctx.ssHeight = sh;

    float quad[] = {
        0,                 0,  0, 0,
        (float)sw,         0,  1, 0,
        (float)sw, (float)sh,  1, 1,

        0,                 0,  0, 0,
        (float)sw, (float)sh,  1, 1,
        0,         (float)sh,  0, 1
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
}