#include "SmoothedValue.h"
#include <math.h>
#include <Arduino.h>

SmoothedValue::SmoothedValue(float alpha, unsigned long minInterval, float minChangeDelta, unsigned long accuracyUpdateInterval)
    : alpha(alpha), minInterval(minInterval),
      step(minChangeDelta), initialized(false), accuracyUpdateInterval(accuracyUpdateInterval) {
  lastChangeTime = 0;
}

float SmoothedValue::update(float newValue) {
    if(step > 1)
    {
        //newValue = round(newValue / step) * step;
    }

    if (!initialized) {
        smoothedValue = newValue;
        lastOutput = newValue;
        lastChangeTime = millis();
        initialized = true;
        return lastOutput;
    }

    smoothedValue = alpha * newValue + (1 - alpha) * smoothedValue;
    float rounded = smoothedValue;

    if (fabs(rounded - lastOutput) >= step || (accuracyUpdateInterval > 0 && (millis() - lastChangeTime) > accuracyUpdateInterval)) {
        unsigned long currentTime = millis();
        if (currentTime - lastChangeTime >= minInterval) {
            lastOutput = rounded;
            lastChangeTime = currentTime;
        }
    }

    return lastOutput;
}