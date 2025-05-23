#include <Wire.h>
#include <SPI.h>
#include <Adafruit_AMG88xx.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

// Кнопки керування та індикація
const int btn_ok = 2;
const int btn_left = 3;
const int btn_right = 4;
const int led = 5;

// SPI інтерфейс
const int tft_rst = 8;
const int tft_dc = 9;
const int tft_cs = 10;

const int tft_sda = 11;
const int tft_scl = 13;

// До сенсора за виключенням I2C
const int tft_int = 12;

// Масив пікселей сенсора
const int gridSize = 8;
const int boxSize = 16;
float pixels[64];

// Створення обʼєкта сенсору 
Adafruit_AMG88xx amg;

// Створення обʼєкта дисплею
Adafruit_ST7735 tft = Adafruit_ST7735(tft_cs, tft_dc, tft_rst);
const int usableHeight = 160 - 32;

// Затримка
unsigned long lastUpdate = 0;
const unsigned long updateInterval = 100;
unsigned long lastUpdateTime = 0;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;
unsigned long del_menu = 0;

// Екран
int currentScreen = -1;
int previousScreen = -2;

// Кнопки
bool btnPressed_ok = true;
bool btnPressed_left = true;
bool btnPressed_right = true;

int lastButtonState_ok = HIGH;
int lastButtonState_left= HIGH;
int lastButtonState_right = HIGH;
int buttonState;

void setup() {
  // Підготовка кнопок
  pinMode (btn_ok, INPUT);
  pinMode (btn_left, INPUT);
  pinMode (btn_right, INPUT);
  pinMode (led, OUTPUT);

  digitalWrite (led, LOW);

  // Початкове налаштування дисплею
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(2);
  tft.fillScreen(ST7735_BLACK);
  tft.setTextColor(ST7735_WHITE);
  tft.setTextSize(2);
  tft.setCursor(24, 30);
  tft.println("Welcome");

  // Перевірка наявності сенсора
  if (!amg.begin(0x68)) {
    tft.fillScreen(ST7735_BLACK);
    tft.setTextColor(ST7735_RED);
    tft.setTextSize(1);
    tft.setCursor(10, 10);
    tft.println("AMG8833 not found!");
    while (true);
  }
  delay (2000);
}

void loop() {
  int reading_ok = digitalRead(btn_ok);
  int reading_left = digitalRead(btn_left);
  int reading_right = digitalRead(btn_right);
  
  unsigned long currentMillis = millis();

  if (reading_ok != lastButtonState_ok) {
    lastDebounceTime = millis();
    del_menu = millis();
  }
    if (reading_left != lastButtonState_left) {
    lastDebounceTime = millis();
  }
    if (reading_right != lastButtonState_right) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading_ok == LOW && !btnPressed_ok) {
      digitalWrite(led,HIGH);
      btnPressed_ok = true;  				// Кнопка натиснута
      drawMenu();
      
    }

    if (currentMillis - del_menu >= 5000){
      tft.fillRect(0, 128, 128, 20, ST7735_BLACK);
    }
    digitalWrite(led,LOW);
  }
  
  lastButtonState_ok = reading_ok;

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading_left == LOW && !btnPressed_left) {
      btnPressed_left = true;  				// Кнопка натиснута
      currentScreen = (currentScreen - 1) % 6;  // Перемикання екранів
    }
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading_right == LOW && !btnPressed_right) {
      btnPressed_right = true;  				// Кнопка натиснута
      currentScreen = (currentScreen + 1) % 6;  // Перемикання екранів
    }
  }

  if (currentMillis - lastUpdate >= updateInterval) { // оновлення кожні 100 мс
    lastUpdate = currentMillis;

    amg.readPixels(pixels); // Зчитування масиву ьемператур 

    for (int y = 0; y < gridSize; y++) {
      for (int x = 0; x < gridSize; x++) {
        int i = (gridSize - 1 - y) * gridSize + (gridSize - 1 - x);
        float temp = pixels[i];

        int xPixel = x * boxSize;
        int yPixel = y * boxSize;

        if (yPixel >= usableHeight) continue;

        // Визначення кольору (градієнт: синій-холодно, червоний-гаряче)
        uint16_t colour = mapTemperatureToColor(temp);

        tft.fillRect(x * boxSize, y * boxSize, boxSize, boxSize, colour);
      }
    }
  }

  if (reading_ok == HIGH) {
    btnPressed_ok = false;
  }

}

uint16_t mapTemperatureToColor(float temp) {
  //  Межі градієнту
  float tMin = 22.0;
  float tMax = 35.0;

  // Обмеження діапазону
  if (temp < tMin) temp = tMin;
  if (temp > tMax) temp = tMax;

  int value = map(temp * 100, tMin * 100, tMax * 100, 0, 255);

  // Червоне – тепле
  uint8_t red = value;
  uint8_t blue = 255 - value;
  uint8_t green = 0;

  return tft.color565(red, green, blue);
}

void drawMenu() {
  tft.fillRect(0, 128, 128, 20, ST7735_BLACK);
  tft.setTextColor(ST7735_WHITE);
  tft.setTextSize(1);
  tft.setCursor(2, 128);
  tft.print("Menu");
}
