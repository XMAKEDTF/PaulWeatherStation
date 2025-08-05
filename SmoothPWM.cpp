// SmoothPWM.cpp
#include "SmoothPWM.h"
#include <avr/io.h>
#include <avr/interrupt.h>

// Статический член класса
SmoothPWM* SmoothPWM::instance = nullptr;

// Конструктор для ручного режима
SmoothPWM::SmoothPWM(int pin, float speed, int pwmMin, int pwmMax)
    : _pwmPin(pin), _pwmMin(pwmMin), _pwmMax(pwmMax)
    , _current(pwmMin), _target(pwmMin)
    , _speed(speed), _accumulator(0.0) {
    
    instance = this;
}

void SmoothPWM::begin() {
    // Настраиваем пин ШИМ как выход
    pinMode(_pwmPin, OUTPUT);
    
    // Инициализируем ШИМ на пине
    analogWrite(_pwmPin, _current);
    
    // Настраиваем таймер
    // от этого таймера помирает I2C
    //setupTimer();
}

void SmoothPWM::setTarget(int target) {
    _target = constrain(target, _pwmMin, _pwmMax);
}

void SmoothPWM::setSpeed(float speed) {
    _speed = speed;
}

int SmoothPWM::getCurrent() const {
    return _current;
}

int SmoothPWM::getTarget() const {
    return _target;
}

void SmoothPWM::updatePWM() {
    // Плавно меняем текущее значение к целевому
    if (abs(_current - _target) >= 2 || _target == _pwmMin) {
        int16_t diff = (int16_t)_target - (int16_t)_current;
        _accumulator += diff * _speed * 0.1f;
        
        int16_t change = (int16_t)_accumulator;
        _accumulator -= change;
        
        int16_t new_value = (int16_t)_current + change;

        if(new_value == 255)
        {
            //тот самый костыль
            new_value = 254;
        }
        
        _current = (int)new_value;
        analogWrite(_pwmPin, _current);
    }
}

void SmoothPWM::setupTimer() {
    cli();
    
    TCCR0A = 0;
    TCCR0B = 0;
    TCNT0 = 0;
    
    // Устанавливаем значение сравнения для частоты ~100 Гц
    OCR0A = 250;
    
    TCCR0A |= (1 << WGM01);
    TCCR0B |= (1 << CS01) | (1 << CS00);
    TIMSK0 |= (1 << OCIE0A);
    
    sei();
}

void SmoothPWM::handleInterrupt() {
    updatePWM();
}

void SmoothPWM::timerISR() {
    if (instance != nullptr) {
        instance->handleInterrupt();
    }
}

ISR(TIMER0_COMPA_vect) {
    SmoothPWM::timerISR();
}