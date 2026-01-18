#ifndef COMMON_HPP_
#define COMMON_HPP_

#include <cmath>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

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

#endif // COMMON_HPP_