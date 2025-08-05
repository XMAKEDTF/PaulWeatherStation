#ifndef SMOOTH_NEO_PIXEL_H
#define SMOOTH_NEO_PIXEL_H

#include <Adafruit_NeoPixel.h>

class SmoothNeoPixel : public Adafruit_NeoPixel {
public:
    SmoothNeoPixel(uint16_t numPixels, uint8_t pin, neoPixelType type = NEO_GRB + NEO_KHZ800, int fps = 60, float smoothTime = 0.3);
    ~SmoothNeoPixel();

    // Убрано override, так как методы в Adafruit_NeoPixel не виртуальные
    void begin();
    void setPixelColor(uint16_t n, uint32_t color);
    void setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b);
    uint32_t getPixelColor(uint16_t n);

    uint32_t lerp(uint32_t a, uint32_t b, float t);

    void setBrightness(uint8_t brightness);
    uint8_t getBrightness();
    void fill(uint32_t color);

    void setSmoothTime(float time);

        
    // Функция для преобразования RGB в HSV
    void rgbToHsv(uint32_t rgb, float &h, float &s, float &v);
    // Функция для преобразования HSV в RGB
    uint32_t hsvToRgb(float h, float s, float v);

    // Явное использование методов базового класса
    using Adafruit_NeoPixel::Color;
    using Adafruit_NeoPixel::ColorHSV;
    using Adafruit_NeoPixel::gamma32;
    using Adafruit_NeoPixel::numPixels;
    
    void update();

private:
    struct Pixel {
        float currentR;
        float currentG;
        float currentB;
        uint8_t targetR;
        uint8_t targetG;
        uint8_t targetB;
    };

    Pixel* pixels;
    float smoothTime;
    bool enabled;
    int FPS;
    float brightness;

    static SmoothNeoPixel* instance;
    static void staticUpdate();
};

#endif