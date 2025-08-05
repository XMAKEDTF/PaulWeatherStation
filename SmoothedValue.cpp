#include "SmoothedValue.h"
#include <math.h>
#include <Arduino.h>

SmoothedValue::SmoothedValue(float alpha, unsigned long minInterval, float minChangeDelta)
    : alpha(alpha), minInterval(minInterval),
      step(minChangeDelta), initialized(false) {
  lastChangeTime = 0;
}

float SmoothedValue::update(float newValue) {
    if (!initialized) {
        smoothedValue = newValue;
        lastOutput = newValue;
        lastChangeTime = millis();
        initialized = true;
        return lastOutput;
    }

    smoothedValue = alpha * newValue + (1 - alpha) * smoothedValue;
    float rounded = smoothedValue;

    if (fabs(rounded - lastOutput) >= step) {
        unsigned long currentTime = millis();
        if (currentTime - lastChangeTime >= minInterval) {
            lastOutput = rounded;
            lastChangeTime = currentTime;
        }
    }

    return lastOutput;
}