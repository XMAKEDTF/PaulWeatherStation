#include <GyverBME280.h>
#include "GyverButton.h"
#include "DFRobot_CCS811.h"
#include "SmoothNeoPixel.h"
#include "BufferedLiquidCrystal.h"
#include "SmoothedValue.h"
#include "SmoothPWM.h"
#include "Graphics.h"
#include <EEPROM.h>

// класс датчика температуры, влажности и давления
GyverBME280 bme;

// класс датчика CO2 и TVOC
DFRobot_CCS811 CCS811;

//включить отображение baseline (для калибровки на чистом воздухе)
#define BASELINE_DEBUG 0
// калибровочное значение для датчика CO2 и TVOC, стандартное в библиотеке - 0x447B, мои калибровочные - ниже (в порядке разных попыток калибровки). 0 - отключить (авто)  (но с авто могут разниться значения от запуска к запуску и другие рофлы)
// B46D
// 146F
// 446F
// значения до этой строки были получены без учета при калибровке значений температуры и влажности, теперь они учитываются
// 5470
// 2473
// c474
// a474
// 4477 - 1476
#define CCS811_BASELINE 0x1476
// сглаживание значений для этого датчика, чем ближе к 0 тем сильнее сглаживание, 1 - нет сглаживания
#define CCS811_SMOOTH 1

//константы уровней для подсветки
const float co2_excellent_min = 450;
const float co2_good_max = 1000;
const float co2_critical_min = 2500;

const float tvoc_excellent_min = 0;
const float tvoc_good_max = 220;
const float tvoc_critical_min = 660;

//константы срабатывания тревоги
const float co2_alarm = 2000;
const float tvoc_alarm = 250;

// пины, куда подключены разные устройства: дисплей, подсветка, датчик уровня освещенности, кнопка, пищалка, светодиодная лента
BufferedLiquidCrystal lcd(7, 8, A0, 10, 11, 12);

#define BL_PIN 9
#define LS_PIN A7
#define BUZ_PIN 6
#define BTN_PIN1 A1
#define BTN_PIN2 A3
#define LED_PIN  4

//количество светодиодов в ленте
#define NUMPIXELS 7

// минимальная и максимальная яркость светодиодной ленты (0-255)
#define LED_MIN 0
#define LED_MAX 128

// коэффицент сглаживания яркости подсветки
#define BL_SMOOTH 0.05
// минимальная и максимальная яркость подсветки дисплея (0-1023)
#define BL_MIN 5
#define BL_MAX 256

// минимальная и максимальная яркость, считываемая с датчика уровня освещенности (0-1023)
#define MIN_BRIGHTNESS 30
#define MAX_BRIGHTNESS 200
// включить отображение текущего уровня освещённости на дисплее
#define BRIGHT_DEBUG 0

// сглаживание отображаемых показаний с датчика BME280 (1 - нет сглаживания)
#define BME280_SMOOTH 1
// смещение температуры по моим тестам и сравнениям с другими устройствами (обусловлено скорее всего нагревом самого устройства)
#define TEMP_OFFS -1.0
// период обновления отображаемых показателей (мс)
#define UPD_RATE 1000

// тут вот эти 1 и 0.1 - минимальная дельта смены значения, чтобы значение не скакало постоянно на пограничных значениях округления
SmoothedValue TempSmt(BME280_SMOOTH, 0, 0.1);
SmoothedValue HumiSmt(BME280_SMOOTH, 0, 1);
SmoothedValue PresSmt(BME280_SMOOTH, 0, 1);

SmoothedValue CO2Smt(CCS811_SMOOTH, 0, 100, 60000);
SmoothedValue TVOCSmt(CCS811_SMOOTH, 0, 10, 60000);

// автоматическая яркость подсветки
SmoothPWM smoothPWM(BL_PIN, BL_SMOOTH, BL_MIN, BL_MAX);

// кнопка
GButton btn(BTN_PIN1);

// светодиодная лента (последняя цифра - время сглаживания (больше - сильнее сглаживание))
SmoothNeoPixel strip(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800, 0, 0.5);

float getCO2Level(float co2_ppm) {
    if (co2_ppm <= co2_excellent_min) {
        return 0.0;
    } else if (co2_ppm <= co2_good_max) {
        return (co2_ppm - co2_excellent_min) / (co2_good_max - co2_excellent_min) * 0.5;
    } else if (co2_ppm <= co2_critical_min) {
        float ratio = (co2_ppm - co2_good_max) / (co2_critical_min - co2_good_max);
        return 0.5 + ratio * 0.5;
    } else {
        return 1.0;
    }
}

float getTVOCLevel(float tvoc_ppb) {
    if (tvoc_ppb <= tvoc_excellent_min) {
        return 0.0;
    } else if (tvoc_ppb <= tvoc_good_max) {
        return (tvoc_ppb / tvoc_good_max) * 0.5;
    } else if (tvoc_ppb <= tvoc_critical_min) {
        float ratio = (tvoc_ppb - tvoc_good_max) / (tvoc_critical_min - tvoc_good_max);
        return 0.5 + ratio * 0.5;
    } else {
        return 1.0;
    }
}

float mapfc(float x, float in_min, float in_max, float out_min, float out_max) {
    x = constrain(x, in_min, in_max);
    float divisor = (in_max - in_min);
    return (x - in_min) * (out_max - out_min) / divisor + out_min;
}

bool canEnableAlarm = true;
// метод который должен вызываться как можно чаще (обработка кнопки и яркости)
void Tick()
{
  btn.tick();

  int sens = 1023 - analogRead(LS_PIN);

  int blBright = mapfc(sens, MIN_BRIGHTNESS, MAX_BRIGHTNESS, BL_MIN, BL_MAX);
  int ledBright = mapfc(sens, MIN_BRIGHTNESS, MAX_BRIGHTNESS, LED_MIN, LED_MAX);

  // я вырубил пищалку так что пускай мигает подсветкой когда угодно
  //canEnableAlarm = sens >= MAX_BRIGHTNESS;

  smoothPWM.setTarget(blBright);

  strip.setBrightness(ledBright);

  smoothPWM.handleInterrupt();
  strip.update();

  digitalWrite(LED_BUILTIN, btn.state());
  
  if(btn.isSingle()) // тестовый писк при нажатии
  {
    tone(BUZ_PIN, 500, 100);
  }
}

void DrawChar(byte customChars[][8], int position, bool useSecondSlot = false) {
    // Определяем начальный адрес для записи символов
    uint8_t baseAddress = useSecondSlot ? 4 : 0;
    
    // Записываем 4 кастомных символа в CGRAM
    for (int i = 0; i < 4; i++) {
        lcd.createChar(baseAddress + i, customChars[i]);
    }
    
    // Определяем позиции для вывода символов (2x2 сетка)
    // Позиция на дисплее: (строка 0 или 1, столбец 0-15)
    // Для 2x2 блока нам нужно определить начальную позицию
    
    int startX = position;  // Позиция по X (0-15)
    int startY = 0;         // По умолчанию начинаем с первой строки

    // Проверяем, чтобы блок 2x2 поместился на экране
    if (startX > 14) {
        startX = 14; // Максимум 14, чтобы 2x2 блок поместился
    }
    
    // Выводим символы 2x2 блоком
    // Верхний ряд
    lcd.setCursor(startX, startY);
    lcd.write((uint8_t)(baseAddress));     // Верхний левый
    lcd.write((uint8_t)(baseAddress + 1)); // Верхний правый
    
    // Нижний ряд
    lcd.setCursor(startX, startY + 1);
    lcd.write((uint8_t)(baseAddress + 2)); // Нижний левый
    lcd.write((uint8_t)(baseAddress + 3)); // Нижний правый
}

void DrawNumber(int number, int positionX) {
  // Определяем первую и вторую цифры числа
  byte firstDigit = 10; // По умолчанию пробел (nS)
  byte secondDigit = 0;

  if (number < 0) {
      // Отрицательное число
      firstDigit = 11; // Минус (nM)
      secondDigit = abs(number); // Вторая цифра - модуль числа
  } else if (number < 10) {
      // Однозначное положительное число
      firstDigit = 10; // Пробел (nS)
      secondDigit = number;
  } else {
      // Двузначное положительное число
      firstDigit = number / 10; // Первая цифра
      secondDigit = number % 10; // Вторая цифра
  }

  // Определяем массивы для первой и второй цифры
  const auto firstChar =  getNumberArray(firstDigit);
  const auto secondChar = getNumberArray(secondDigit);

  // Выводим первую цифру
  DrawChar(firstChar, positionX, false);

  // Выводим вторую цифру
  DrawChar(secondChar, positionX + 2, true);
}

void Draw(float temp, float humi, float pres, float CO2, float TVOC, bool ticker)
{
  lcd.setCursor(4, 0);
  lcd.write((uint8_t)0xEF);
  lcd.print("C");
  lcd.setCursor(3, 1);
  float s = temp - ((int)(temp));
  if(s < 0)
  {
    s = -s;
  }
  lcd.print(String(s, 1));

  DrawNumber((int)(temp), 0);

  lcd.setCursor(13, 1);
  lcd.print(String(pres, 0));

  lcd.setCursor(13, 0);
  lcd.print(String(humi, 0));
  lcd.print("%");

  if(TVOC < tvoc_alarm || ticker)
  {
    lcd.setCursor(8, 1);
    lcd.print(String((int)TVOC));
  }

  if(CO2 < co2_alarm || !ticker)
  {
    lcd.setCursor(8, 0);
    lcd.print(String((int)CO2));
  }
}

void setup() 
{
  // Пины D9 и D10 - 15.6 кГц 10bit
  TCCR1A = 0b00000011;  // 10bit
  TCCR1B = 0b00001001;  // x1 fast pwm

  pinMode(BL_PIN, OUTPUT);
  pinMode(LS_PIN, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(BUZ_PIN, OUTPUT);
  pinMode(BTN_PIN2, OUTPUT);
  digitalWrite(BTN_PIN2, LOW);

  digitalWrite(LED_BUILTIN, LOW);

  strip.begin();
  strip.show();

  lcd.begin(16, 2);

  lcd.clear();
  lcd.display();

  smoothPWM.begin();

  bme.begin(0x76);

  CCS811.begin();

  delay(500); //иначе будут нули в первый апд
}

void loop() 
{
  static bool ticker;
  ticker = !ticker;

  unsigned long startTime = millis();

  static float temp;
  static float humi;
  static float pres;

  static float CO2;  // ppm
  static float CO2_raw;  // ppm
  static float TVOC; // ppb
  static float TVOC_raw; // ppb

  // читаем сенсоры по тикрейту, а обновляем всё остальное в 2 раза чаще - чтобы реализовать мигание в предупреждении о высоком уровне
  if(ticker)
  {
    const auto temp_raw = bme.readTemperature();
    const auto humi_raw = bme.readHumidity();

    temp = TempSmt.update(temp_raw + TEMP_OFFS);
    humi = HumiSmt.update(humi_raw);
    pres = PresSmt.update(bme.readPressure() / 133.322); //перевод давления из паскалей в мм ртутного столба

    Tick();

    CCS811.setInTempHum(temp_raw, humi_raw);

    Tick();

    if(CCS811.checkDataReady())
    {
      CO2_raw = CCS811.getCO2PPM();
      CO2 = CO2Smt.update(CO2_raw);
      TVOC_raw = CCS811.getTVOCPPB();
      TVOC = TVOCSmt.update(TVOC_raw);
    }

    Tick();

    if(!BASELINE_DEBUG && CCS811_BASELINE)
    {
      CCS811.writeBaseLine(CCS811_BASELINE);
    }
  }

  bool alarm_raw = CO2_raw >= co2_alarm || TVOC_raw >= tvoc_alarm;
  bool alarm = CO2 >= co2_alarm || TVOC >= tvoc_alarm;
  alarm &= alarm_raw && canEnableAlarm;

  float CO2_level = getCO2Level(CO2);
  float TVOC_level = getTVOCLevel(TVOC);

  strip.fill(0);

  float level_max = pow(max(CO2_level, TVOC_level), 0.7);
  uint16_t pixelHue = (0.33333 - level_max * 0.33333) * 65536L;
  auto color = strip.gamma32(strip.ColorHSV(pixelHue));
  
  if(alarm && !ticker)
  {
    color = 0;
  }

  if(!alarm)
  {
    //крч решил только 2 крайних светодиода включать - эффект почти такой же как от всех но меньше влияние на датчик температуры
    strip.setPixelColor(NUMPIXELS-1, color);
    //strip.setPixelColor(NUMPIXELS/2, color);
    strip.setPixelColor(0, color);
  }
  else{
    strip.fill(color);
    //strip.setPixelColor(4, 0); // чтобы не мешать измерениям температуры: этот светодиод прям около датчика

    if(ticker)
    {
      //пищим (уже не пищим)
      //tone(BUZ_PIN, 1000, UPD_RATE / 2);
    }
  }

  lcd.clear();

  Tick();

  Draw(temp, humi, pres, CO2, TVOC, ticker);

  if(BRIGHT_DEBUG)
  {
    lcd.setCursor(0,0);
    lcd.print(String(1023-analogRead(LS_PIN)));
    lcd.print("   ");
  }
  else if(BASELINE_DEBUG)
  {
    lcd.setCursor(0,0);
    lcd.print(CCS811.readBaseLine(), HEX);
    lcd.print("   ");
  }

  Tick();
  
  lcd.display();

  while(millis() - startTime < (UPD_RATE / 2) && millis() >= startTime)
  {
    Tick();
    delay(5);
  }

  Tick();
}