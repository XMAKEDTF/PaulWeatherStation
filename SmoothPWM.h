// SmoothPWM.h
#ifndef SmoothPWM_h
#define SmoothPWM_h

#include <Arduino.h>

class SmoothPWM {
public:
    // Альтернативный конструктор для ручного управления
    SmoothPWM(int pin, float speed = 1.0, int pwmMin = 0, int pwmMax = 255);
    
    // Инициализация
    void begin();
    
    // Установка целевого значения ШИМ (0-255) - для ручного режима
    void setTarget(int target);
    
    // Установка скорости сглаживания (0.01 - 10.0)
    void setSpeed(float speed);
    
    // Получение текущего значения ШИМ
    int getCurrent() const;
    
    // Получение целевого значения ШИМ
    int getTarget() const;
    
    // Переключение между автоматическим и ручным режимом
    void setAutoMode(bool autoMode);
    
    // Обработчик прерывания таймера
    void handleInterrupt();
    
    // Статический метод для обработчика прерывания
    static void timerISR();

private:
    // Пины
    int _pwmPin;

    // Настройки ШИМ
    int _current;
    int _target;
    int _pwmMin;
    int _pwmMax;
    
    // Настройки сглаживания
    float _speed;
    float _accumulator;
    
    // Статический указатель на текущий экземпляр
    static SmoothPWM* instance;
    
    // Вспомогательные методы
    void updatePWM();
    void setupTimer();
};

#endif