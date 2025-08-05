#include "SmoothNeoPixel.h"

SmoothNeoPixel* SmoothNeoPixel::instance = nullptr;

SmoothNeoPixel::SmoothNeoPixel(uint16_t numPixels, uint8_t pin, neoPixelType type, int fps, float smoothTime)
    : Adafruit_NeoPixel(numPixels, pin, type), pixels(new Pixel[numPixels]), smoothTime(smoothTime), enabled(false), FPS(fps), brightness(1) {
    for (int i = 0; i < numPixels; i++) {
        pixels[i].currentR = 0.0f;
        pixels[i].currentG = 0.0f;
        pixels[i].currentB = 0.0f;
        pixels[i].targetR = 0;
        pixels[i].targetG = 0;
        pixels[i].targetB = 0;
    }
    instance = this;
}

SmoothNeoPixel::~SmoothNeoPixel() {
    delete[] pixels;
}

void SmoothNeoPixel::begin() {
    Adafruit_NeoPixel::begin(); // Вызов метода базового класса
    enabled = true;
    if(FPS > 0)
    {
        //timer.attach_ms(1000 / FPS, SmoothNeoPixel::staticUpdate);
        //TODO: сделать по прерываниям для ардуины? пока что не нужно и лень
    }
    Adafruit_NeoPixel::setBrightness(255);
}

void SmoothNeoPixel::setPixelColor(uint16_t n, uint32_t color) {
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    setPixelColor(n, r, g, b);
}

void SmoothNeoPixel::setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b) {
    if (n >= numPixels()) return;
    pixels[n].targetR = r;
    pixels[n].targetG = g;
    pixels[n].targetB = b;
}

uint32_t SmoothNeoPixel::getPixelColor(uint16_t n)
{
    if (n >= numPixels()) {
        return 0; // Возвращаем черный цвет, если индекс вне диапазона
    }

    // Получаем текущие значения RGB из структуры Pixel
    uint8_t r = static_cast<uint8_t>(pixels[n].targetR);
    uint8_t g = static_cast<uint8_t>(pixels[n].targetG);
    uint8_t b = static_cast<uint8_t>(pixels[n].targetB);

    // Формируем и возвращаем цвет в формате uint32_t
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

uint32_t SmoothNeoPixel::lerp(uint32_t A, uint32_t B, float t)
{
    uint8_t ra = (A >> 16) & 0xFF;
    uint8_t ga = (A >> 8) & 0xFF;
    uint8_t ba = A & 0xFF;

    uint8_t rb = (B >> 16) & 0xFF;
    uint8_t gb = (B >> 8) & 0xFF;
    uint8_t bb = B & 0xFF;

    float ti = 1 - t;

    // Получаем текущие значения RGB
    uint8_t r = static_cast<uint8_t>(ra * ti + rb * t);
    uint8_t g = static_cast<uint8_t>(ga * ti + gb * t);
    uint8_t b = static_cast<uint8_t>(ba * ti + bb * t);

    // Формируем и возвращаем цвет в формате uint32_t
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

void SmoothNeoPixel::fill(uint32_t color)
{
    for(int i = 0; i < numPixels(); i++)
    {
        setPixelColor(i, color);
    }
}

void SmoothNeoPixel::setBrightness(uint8_t b) {
    brightness = ((float)b) / 255.0F;
}

uint8_t SmoothNeoPixel::getBrightness()
{
    return brightness * 255;
}

void SmoothNeoPixel::setSmoothTime(float time) {
    smoothTime = time;
}

void SmoothNeoPixel::staticUpdate() {
    if (instance) {
        instance->update();
    }
}


// Функция для преобразования RGB в HSV
void SmoothNeoPixel::rgbToHsv(uint32_t rgb, float &h, float &s, float &v) {
    uint8_t r = (rgb >> 16) & 0xFF;
    uint8_t g = (rgb >> 8) & 0xFF;
    uint8_t b = rgb & 0xFF;

    float delta, min;
    float hue = 0, saturation, value;

    min = r < g ? (r < b ? r : b) : (g < b ? g : b);
    value = r > g ? (r > b ? r : b) : (g > b ? g : b);
    delta = value - min;

    if (value != 0) {
        saturation = delta / value;
    } else {
        saturation = 0;
        hue = -1;
        h = hue;
        s = saturation;
        v = value / 255.0;
        return;
    }

    if (r == value) {
        hue = (g - b) / delta;
    } else if (g == value) {
        hue = 2 + (b - r) / delta;
    } else {
        hue = 4 + (r - g) / delta;
    }

    hue *= 60;
    if (hue < 0) hue += 360;

    h = hue / 360.0; // Нормализуем hue к диапазону [0, 1]
    s = saturation;
    v = value / 255.0;
}

// Функция для преобразования HSV в RGB
uint32_t SmoothNeoPixel::hsvToRgb(float h, float s, float v) {
    int i;
    float f, p, q, t, r, g, b;

    if (s == 0) {
        r = g = b = v * 255;
        return ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
    }

    h *= 6.0; // Умножаем hue на 6, чтобы получить индекс сектора
    i = floor(h);
    f = h - i;
    p = v * (1 - s);
    q = v * (1 - s * f);
    t = v * (1 - s * (1 - f));

    switch (i) {
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        case 5: r = v; g = p; b = q; break;
    }

    return ((uint32_t)(r * 255) << 16) | ((uint32_t)(g * 255) << 8) | (uint32_t)(b * 255);
}

void SmoothNeoPixel::update() {
    if (!enabled) return;

    static unsigned long lt;

    float deltaTime = (millis() - lt) / 1000.0;
    lt = millis();
    float smoothFactor = deltaTime / smoothTime;

    bool anyChanged = false;

    for (int i = 0; i < numPixels(); i++) {
        Pixel& p = pixels[i];

        // Плавное обновление цветов
        p.currentR += ((p.targetR * brightness) - p.currentR) * smoothFactor;
        p.currentR = constrain(p.currentR, 0.0f, 255.0f);
        p.currentG += ((p.targetG * brightness) - p.currentG) * smoothFactor;
        p.currentG = constrain(p.currentG, 0.0f, 255.0f);
        p.currentB += ((p.targetB * brightness) - p.currentB) * smoothFactor;
        p.currentB = constrain(p.currentB, 0.0f, 255.0f);

        // Применение яркости и установка цвета
        uint8_t r = (p.currentR);
        uint8_t g = (p.currentG);
        uint8_t b = (p.currentB);

        // Формируем и возвращаем цвет в формате uint32_t
        uint32_t nc = ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
        uint32_t last = Adafruit_NeoPixel::getPixelColor(i);

        if(nc != last)
        {
            anyChanged = true;
        }

        Adafruit_NeoPixel::setPixelColor(i, nc);
    }
    
    if(anyChanged)
    {
        Adafruit_NeoPixel::show();
    }
}