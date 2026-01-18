// #include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <d3d11.h>
#include <dxgi1_2.h>
#include <cstdio>

// #pragma comment(lib, "d3d11.lib")
// #pragma comment(lib, "dxgi.lib")
// #pragma comment(lib, "dxguid.lib")

#define CHECK(hr, msg) \
do { \
    if (FAILED(hr)) { \
        fprintf(stderr, "%s failed (0x%08X)\n", msg, (unsigned)hr); \
        exit(EXIT_FAILURE); \
    } \
} while (0)

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <cmath>
#include <vector>
#include <atomic>
#include <thread>

// // Enable Accelerated GPU
// #ifdef _WIN32

// extern "C" {
//     __declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
//     __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
// }

// #endif // _WIN32

#include "gl_utils.hpp"
#include "nav.hpp"
#include "config.hpp"

using thread = std::thread;

template<typename T>
using atomic = std::atomic<T>;

template<typename T>
using vector = std::vector<T>;

struct DxgiCapture {
    ID3D11Device* device;
    ID3D11DeviceContext* context;

    IDXGIOutputDuplication* duplication;

    ID3D11Texture2D* staging;
    int width;
    int height;

    std::uint8_t* image;
    int pitch;
    IDXGIResource* resource;
    ID3D11Texture2D* desktop;
};

DxgiCapture initDxgiCapture() {
    DxgiCapture cap{};

    HRESULT hr;

    hr = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr, 0,
        nullptr, 0,
        D3D11_SDK_VERSION,
        &cap.device,
        nullptr,
        &cap.context
    );
    CHECK(hr, "D3D11CreateDevice");

    IDXGIDevice* dxgi_device = nullptr;
    hr = cap.device->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgi_device);
    CHECK(hr, "QueryInterface IDXGIDevice");

    IDXGIAdapter* adapter = nullptr;
    hr = dxgi_device->GetAdapter(&adapter);
    CHECK(hr, "GetAdapter");

    IDXGIOutput* output = nullptr;
    hr = adapter->EnumOutputs(0, &output);
    CHECK(hr, "EnumOutputs");

    IDXGIOutput1* output1 = nullptr;
    hr = output->QueryInterface(__uuidof(IDXGIOutput1), (void**)&output1);
    CHECK(hr, "QueryInterface IDXGIOutput1");

    hr = output1->DuplicateOutput(cap.device, &cap.duplication);
    CHECK(hr, "DuplicateOutput");

    IDXGIResource* resource = nullptr;
    DXGI_OUTDUPL_FRAME_INFO info = {};

    hr = cap.duplication->AcquireNextFrame(
        0,
        &info,
        &resource
    );
    CHECK(hr, "AcquireNextFrame");

    ID3D11Texture2D* texture = nullptr;
    hr = resource->QueryInterface(
        __uuidof(ID3D11Texture2D),
        (void**)&texture
    );
    CHECK(hr, "QueryInterface ID3D11Texture2D");

    D3D11_TEXTURE2D_DESC desc;
    texture->GetDesc(&desc);

    if (desc.Format != DXGI_FORMAT_B8G8R8A8_UNORM) {
        fprintf(stderr, "Unexpected desktop format\n");
    }

    cap.width = desc.Width;
    cap.height = desc.Height;

    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.MiscFlags = 0;

    printf("Format = %d\n", desc.Format);

    cap.device->CreateTexture2D(&desc, nullptr, &cap.staging);

    cap.duplication->ReleaseFrame();
    texture->Release();
    resource->Release();
    output1->Release();
    output->Release();
    adapter->Release();
    dxgi_device->Release();

    return cap;
}

bool tryCapture(DxgiCapture& cap) {
    IDXGIResource* resource = nullptr;
    DXGI_OUTDUPL_FRAME_INFO info {};

    HRESULT hr;
    hr = cap.duplication->AcquireNextFrame(
        0,
        &info,
        &resource
    );

    if (hr == DXGI_ERROR_WAIT_TIMEOUT) return false;

    if (FAILED(hr)) return false;

    if (info.AccumulatedFrames == 0) {
        cap.duplication->ReleaseFrame();
        resource->Release();
        return false;
    }

    ID3D11Texture2D* desktop = nullptr;
    hr = resource->QueryInterface(
        __uuidof(ID3D11Texture2D),
        (void**)&desktop
    );
    CHECK(hr, "QueryInterface ID3D11Texture2D");

    cap.context->CopyResource(cap.staging, desktop);
    cap.context->Flush();

    D3D11_MAPPED_SUBRESOURCE map;
    hr = cap.context->Map(cap.staging, 0, D3D11_MAP_READ, 0, &map);
    CHECK(hr, "Map");

    cap.image = static_cast<std::uint8_t*>(map.pData);

    cap.pitch = map.RowPitch;
    // Assign desktop and resource and release them only in the endCapture function
    cap.desktop = desktop;
    cap.resource = resource;
    return true;
}

void endCapture(DxgiCapture& cap) {
    cap.context->Unmap(cap.staging, 0);
    cap.duplication->ReleaseFrame();
    cap.desktop->Release();
    cap.resource->Release();
    cap.image = nullptr;
}

void destroyDxgiCapture(DxgiCapture& cap) {
    cap.staging->Release();
    cap.duplication->Release();
    cap.context->Release();
    cap.device->Release();
}

#include "common.hpp"

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
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

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

    HWND hwnd = glfwGetWin32Window(window);

    LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
    style &= ~(WS_OVERLAPPEDWINDOW);
    style |= WS_POPUP;
    SetWindowLongPtr(hwnd, GWL_STYLE, style);

    LONG_PTR exStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
    exStyle |= WS_EX_TOOLWINDOW;
    SetWindowLongPtr(hwnd, GWL_EXSTYLE, exStyle);

    SetWindowPos(
        hwnd,
        HWND_TOPMOST,
        0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED
    );

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

    GLuint screenTex;
    glGenTextures(1, &screenTex);
    glBindTexture(GL_TEXTURE_2D, screenTex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    DxgiCapture screen = initDxgiCapture();
    atomic<bool> running = true;

    thread captureThread([&]{
        while (running) {
            if (tryCapture(screen)) {
                printf("Screenshot size: %d x %d\n", screen.width, screen.height);

                glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                glPixelStorei(GL_UNPACK_ROW_LENGTH, screen.pitch / 4);

                // uint32_t firstPixel = screen.image[3] | (screen.image[2] << 8) | (screen.image[1] << 16) | (screen.image[0] << 24);
                // printf("First pixel = 0x%08X\n", firstPixel);

                glTexImage2D(GL_TEXTURE_2D,
                    0,
                    GL_RGBA8,
                    screen.width,
                    screen.height,
                    0,
                    GL_BGRA,
                    GL_UNSIGNED_BYTE,
                    screen.image
                );

                glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

                endCapture(screen);

                running = false;
            }
        }
    });

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

    glfwShowWindow(window);

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

    destroyDxgiCapture(screen);
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteTextures(1, &screenTex);
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}