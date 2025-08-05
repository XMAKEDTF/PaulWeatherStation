#ifndef BufferedLiquidCrystal_h
#define BufferedLiquidCrystal_h

#include <LiquidCrystal.h>
#include <Arduino.h>

class BufferedLiquidCrystal : public LiquidCrystal {
private:
    uint8_t _cols;
    uint8_t _rows;
    char* _buffer;
    bool _bufferChanged;
    uint8_t _cursorCol;
    uint8_t _cursorRow;

public:
    // Конструкторы
    BufferedLiquidCrystal(uint8_t rs, uint8_t enable,
                          uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3)
        : LiquidCrystal(rs, enable, d0, d1, d2, d3) {
        //init(16, 2);
    }

    BufferedLiquidCrystal(uint8_t rs, uint8_t enable,
                          uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3,
                          uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7)
        : LiquidCrystal(rs, enable, d0, d1, d2, d3, d4, d5, d6, d7) {
        //init(16, 2);
    }

    BufferedLiquidCrystal(uint8_t rs, uint8_t rw, uint8_t enable,
                          uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3)
        : LiquidCrystal(rs, rw, enable, d0, d1, d2, d3) {
        //init(16, 2);
    }

    BufferedLiquidCrystal(uint8_t rs, uint8_t rw, uint8_t enable,
                          uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3,
                          uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7)
        : LiquidCrystal(rs, rw, enable, d0, d1, d2, d3, d4, d5, d6, d7) {
        //init(16, 2);
    }

    // Инициализация с заданными размерами
    void init(uint8_t cols, uint8_t rows) {
        _cols = cols;
        _rows = rows;
        _buffer = new char[cols * rows];
        _bufferChanged = true;
        _cursorCol = 0;
        _cursorRow = 0;
        clear();
    }

    // Переопределение begin для установки размеров
    void begin(uint8_t cols, uint8_t rows) {
        LiquidCrystal::begin(cols, rows);
        init(cols, rows);
    }
    
    void createChar(uint8_t location, uint8_t charmap[]) {
        location &= 0x7; // we only have 8 locations 0-7
        LiquidCrystal::command(LCD_SETCGRAMADDR | (location << 3));
        for (int i=0; i<8; i++) {
            LiquidCrystal::write(charmap[i]);
        }
    }

    // Установка курсора
    void setCursor(uint8_t col, uint8_t row) {
        if (row >= _rows) row = _rows - 1;
        if (col >= _cols) col = _cols - 1;
        _cursorCol = col;
        _cursorRow = row;
    }

    // Очистка только буфера
    void clear() {
        for (int i = 0; i < _cols * _rows; i++) {
            _buffer[i] = ' ';
        }
        _bufferChanged = true;
        _cursorCol = 0;
        _cursorRow = 0;
    }

    // Получение индекса в буфере по текущим координатам
    int getBufferIndex() {
        return _cursorRow * _cols + _cursorCol;
    }

    // Печать символа в буфер
    virtual size_t write(uint8_t value) {
        if (_cursorRow >= _rows) return 0;
        if (_cursorCol >= _cols) return 0;

        int index = getBufferIndex();
        if (index < _cols * _rows) {
            _buffer[index] = (char)value;
            _bufferChanged = true;
        }

        // Перемещение курсора
        _cursorCol++;
        if (_cursorCol >= _cols) {
            _cursorCol = 0;
            _cursorRow++;
            if (_cursorRow >= _rows) {
                _cursorRow = 0; // или оставляем на последней строке
            }
        }

        return 1;
    }

    // Печать строки в буфер
    void print(const char str[]) {
        if (str == nullptr) return;
        for (int i = 0; str[i] != '\0'; i++) {
            write((uint8_t)str[i]);
        }
    }

    void print(const String &s) {
        print(s.c_str());
    }

    void print(char c, uint8_t base = DEC) {
        write((uint8_t)c);
    }

    void print(int n, uint8_t base = DEC) {
        char buffer[16];
        itoa(n, buffer, base);
        print(buffer);
    }

    void print(unsigned int n, uint8_t base = DEC) {
        char buffer[16];
        utoa(n, buffer, base);
        print(buffer);
    }

    void print(long n, uint8_t base = DEC) {
        char buffer[16];
        ltoa(n, buffer, base);
        print(buffer);
    }

    void print(unsigned long n, uint8_t base = DEC) {
        char buffer[16];
        ultoa(n, buffer, base);
        print(buffer);
    }

    void print(double n, uint8_t digits = 2) {
        char buffer[32];
        dtostrf(n, 0, digits, buffer);
        print(buffer);
    }

    // Отправка содержимого буфера на дисплей
    void display() {
        if (!_bufferChanged) return;

        //LiquidCrystal::clear();

        for (uint8_t row = 0; row < _rows; row++) {
            LiquidCrystal::setCursor(0, row);
            for (uint8_t col = 0; col < _cols; col++) {
                int index = row * _cols + col;
                LiquidCrystal::write(_buffer[index]);
            }
        }

        _bufferChanged = false;
    }

    // Деструктор
    ~BufferedLiquidCrystal() {
        delete[] _buffer;
    }
};

#endif