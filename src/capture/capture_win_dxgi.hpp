#pragma once

#include <cstdint>

class DXGIScreenCapture {
public:
    DXGIScreenCapture();
    ~DXGIScreenCapture();

    bool isValid() const;

    int width() const;
    int height() const;

    bool capture();

    const uint8_t* data() const;
private:
    struct Impl;
    Impl* impl;
};