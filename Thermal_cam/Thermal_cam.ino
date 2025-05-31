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

const float minWindowSize = 5.0;
const float maxWindowSize = 20.0;
const float antiNoise = 3.15;

float tMin = 22;
float tMax = 30;

float filtredMinT = tMin;
float filtredMaxT = tMax;

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
int menuIndex = 0;
int submenuIndex = 0;

bool menuVisible = false;
bool inSubmenu = false;

// Режими екранів
const char* mainMenu[] = {"Palette", "Text Temp." , "Pause"};
const int mainMenuSize = sizeof(mainMenu)/ sizeof(mainMenu[0]);

const char* colourModes[] = {"White-hot", "Black-hot", "Red-hot", "Red/blue", "Rainbow"};
const int colourModesSizes = sizeof(colourModes)/ sizeof(colourModes[0]);

int currentColourMode = 0;

const char* tempModes[] = {"Clear", "Min/Max/Avg", "Center temp.", "All info"};
const int tempModesSizes = sizeof(tempModes)/ sizeof(tempModes[0]);

int currentTempMode = 0;

bool isPaused = false;

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
    del_menu = millis();
  }
    if (reading_right != lastButtonState_right) {
    lastDebounceTime = millis();
    del_menu = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading_ok == LOW && !btnPressed_ok) {
      digitalWrite(led,HIGH);
      btnPressed_ok = true;  				// Кнопка натиснута
      
      if (!menuVisible){
        menuVisible = true;
        drawMenu();
      } else if (!inSubmenu){
        submenuIndex = 0;
        if(menuIndex == 2){
          isPaused = !isPaused;
          inSubmenu = false;
          drawMenu();
        }else {
        inSubmenu = true;
        drawSubmenu ();
        }
      } else {
        if (menuIndex == 0){
          currentColourMode = submenuIndex;
          inSubmenu = false;
          drawMenu();
        }else if(menuIndex == 1){
          currentTempMode = submenuIndex;
          tft.fillRect(74, 128, 128, 32, ST7735_BLACK);
          tft.fillRect(0, 128, 128, 32, ST7735_BLACK);
          inSubmenu = false;
          drawMenu();
        }
      }

    }

    if (currentMillis - del_menu >= 5000){
      tft.fillRect(0, 128, 74, 8, ST7735_BLACK);
    }
    digitalWrite(led,LOW);
  }
  
  lastButtonState_ok = reading_ok;

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading_left == LOW && !btnPressed_left) {
      btnPressed_left = true;
      digitalWrite(led,HIGH);
      
      if (menuVisible){
        if (!inSubmenu){
          menuIndex = (menuIndex - 1 + mainMenuSize) % mainMenuSize;
          drawMenu();
        }else {
          if (menuIndex == 0){
            submenuIndex = (submenuIndex - 1 + colourModesSizes) % colourModesSizes;
            drawSubmenu ();
          }else if (menuIndex == 1){
            submenuIndex = (submenuIndex - 1 + tempModesSizes) % tempModesSizes;
            drawSubmenu ();
          }
        }
      }
    }

    if (currentMillis - del_menu >= 5000 && menuVisible){
      tft.fillRect(0, 128, 74, 8, ST7735_BLACK);
      menuVisible = false;
      inSubmenu = false;
    }

    digitalWrite(led,LOW);
  }

  lastButtonState_left = reading_left;

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading_right == LOW && !btnPressed_right) {
      btnPressed_right = true;
      digitalWrite(led,HIGH);

      if (menuVisible){
        if (!inSubmenu){
          menuIndex = (menuIndex + 1) % mainMenuSize;
          drawMenu();
        }else {
          if (menuIndex == 0){
            submenuIndex = (submenuIndex + 1) % colourModesSizes;
            drawSubmenu ();
          }else if (menuIndex == 1){
            submenuIndex = (submenuIndex + 1) % tempModesSizes;
            drawSubmenu ();
          }
        }
      }
    }

    if (currentMillis - del_menu >= 5000 && menuVisible){
      tft.fillRect(0, 128, 74, 8, ST7735_BLACK);
      menuVisible = false;
      inSubmenu = false;
    }
    
    digitalWrite(led,LOW);
  }

  lastButtonState_right = reading_right;

  if (!isPaused && currentMillis - lastUpdate >= updateInterval) { // оновлення кожні 100 мс
    lastUpdate = currentMillis;

    amg.readPixels(pixels); // Зчитування масиву температур 

    float rawMinT = 999;
    float rawMaxT = -999;
    float sumT = 0;

    float centerT = pixels[4 * gridSize + 4];

    for (int i = 0; i < 64; i++){
      sumT += pixels[i];
      if (pixels [i] < rawMinT){
        rawMinT = pixels[i];
      }
      if (pixels [i] > rawMaxT){
        rawMaxT = pixels[i];
      }
    }

    float avgT = sumT / 64;
    float range = rawMaxT - rawMinT;

    float desiredMin = 0;
    float desiredMax = 0;

    if (range < minWindowSize){
      desiredMin = antiNoise + avgT - minWindowSize / 2.0;
      desiredMax = avgT + minWindowSize / 2.0;
    }else if (range > maxWindowSize){
      desiredMin = rawMaxT - maxWindowSize;
      desiredMax = rawMaxT;
    }else {
      desiredMin = rawMinT;
      desiredMax = rawMaxT;
    }

    tMin = 0.9 * tMin + 0.1 * desiredMin;
    tMax = 0.9 * tMax + 0.1 * desiredMax;

    for (int y = 0; y < gridSize; y++) {
      for (int x = 0; x < gridSize; x++) {
        int i = (gridSize - 1 - y) * gridSize + (gridSize - 1 - x);
        float temp = pixels[i];

        int xPixel = x * boxSize;
        int yPixel = y * boxSize;

        if (yPixel >= usableHeight) continue;

        uint16_t colour = mapTemperatureToColor(temp, tMin, tMax);

        tft.fillRect(x * boxSize, y * boxSize, boxSize, boxSize, colour);
      }
    }
    
    drawTemp(rawMinT, rawMaxT, avgT, centerT);

  }

  if (reading_ok == HIGH) {
    btnPressed_ok = false;
  }
  if (reading_left == HIGH) {
    btnPressed_left = false;
  }
  if (reading_right == HIGH) {
    btnPressed_right = false;
  }
  
}

uint16_t mapTemperatureToColor(float temp, float tMin, float tMax) {

  // Обмеження діапазону
  if (temp < tMin) temp = tMin;
  if (temp > tMax) temp = tMax;

  int value = map(temp * 100, tMin * 100, tMax * 100, 0, 255);

  switch(currentColourMode) {
    case 0:
      return tft.color565(value, value, value); // Червоне – тепле
      break;
    case 1:
      return tft.color565(255 - value, 255 - value, 255 - value); // Біле – тепле
      break; 
    case 2:
      return tft.color565(value, value/4, 0); //  Червоне – тепле
      break;
    case 3:
      return tft.color565(value, 0, 255 - value); // Червоне – тепле, синє холодне
      break; 
    case 4:
      float f = value / 255.0f;
      if (f<0.33f){
        float t = f/0.33f;
        return tft.color565((1-t) * 128 + t * 0, 0, (1-t) * 128 + t * 255);
      } else if (f<0.66f){
        float t = (f - 0.33f) / 0.33f;
        return tft.color565(t * 255, t * 255, (1-t) * 255);
      }else {
        float t = (f - 0.66f) / 0.34f;
        return tft.color565( 255, (1 - t) * 255, 0);
      }
      break; 
    default:
      return ST7735_BLACK;
    break;
  }
}

void drawMenu() {
  tft.fillRect(0, 128, 74, 8, ST7735_BLACK);
  tft.setTextColor(ST7735_WHITE);
  tft.setTextSize(1);
  tft.setCursor(2, 128);
  tft.print("<");
  tft.print(mainMenu[menuIndex]);
  tft.print(">");
}

void drawSubmenu (){
  tft.fillRect(0, 128, 74, 8, ST7735_BLACK);
  tft.setTextColor(ST7735_WHITE);
  tft.setTextSize(1);
  tft.setCursor(2, 128);

  if(menuIndex == 0){
    tft.print(colourModes[submenuIndex]);
  }else if(menuIndex == 1){
    tft.print(tempModes[submenuIndex]);
  }
}

void drawTemp (float rawMinT, float rawMaxT, float avgT, float centerT){
  tft.setTextColor(ST7735_WHITE);

    if (currentTempMode == 0) {
    // Clear: нічого не малюємо
  }

  if (currentTempMode == 1) {
    // Min/Max/Avg
    tft.setTextSize(1);
    tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
    tft.setCursor(74, 130);
    tft.print("Min:");
    tft.print(rawMinT, 1);
    tft.setCursor(74, 140);
    tft.print("Max:");
    tft.print(rawMaxT, 1);
    tft.setCursor(74, 150);
    tft.print("Avg:");
    tft.print(avgT, 1);
  }

  if (currentTempMode == 2) {
    // Center Temp
    tft.setTextSize(2);
    tft.setTextColor(ST7735_YELLOW, ST7735_BLACK);
    tft.setCursor(40, 140);
    tft.print(centerT, 1);
  }

  if (currentTempMode == 3) {
    // All Info
    tft.setTextSize(1);
    tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
    tft.setCursor(74, 130);
    tft.print("Min:");
    tft.print(rawMinT, 1);
    tft.setCursor(74, 140);
    tft.print("Max:");
    tft.print(rawMaxT, 1);
    tft.setCursor(74, 150);
    tft.print("Avg:");
    tft.print(avgT, 1);

    tft.setTextSize(2);
    tft.setTextColor(ST7735_YELLOW, ST7735_BLACK);
    tft.setCursor(20, 140);
    tft.print(centerT, 1);
  }
}
