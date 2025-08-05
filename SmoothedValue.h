#ifndef SMOOTHED_VALUE_H
#define SMOOTHED_VALUE_H

class SmoothedValue {
public:
    SmoothedValue(float alpha, unsigned long minInterval = 0, float minChangeDelta = 0);
    float update(float newValue);

private:
    float alpha;
    float step;
    float smoothedValue;
    float lastOutput;
    unsigned long minInterval;
    unsigned long lastChangeTime;
    bool initialized;
};

#endif