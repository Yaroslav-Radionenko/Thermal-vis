#include <Wire.h>
#include <SPI.h>
#include <Adafruit_AMG88xx.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

// Control buttons and indicator
const int btn_ok = 2;
const int btn_left = 3;
const int btn_right = 4;
const int led = 5;

// SPI interface
const int tft_rst = 8;
const int tft_dc = 9;
const int tft_cs = 10;

const int tft_sda = 11;
const int tft_scl = 13;

// To the sensor, except for I2C
const int tft_int = 12;

// Sensor pixel array
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

// Create sensor object
Adafruit_AMG88xx amg;

// Create display object
Adafruit_ST7735 tft = Adafruit_ST7735(tft_cs, tft_dc, tft_rst);
const int usableHeight = 160 - 32;

// Delay
unsigned long lastUpdate = 0;
const unsigned long updateInterval = 100;
unsigned long lastUpdateTime = 0;

unsigned long lastDebounceTime = 0;

const unsigned long debounceDelay = 50;
unsigned long del_menu = 0;

// Screen
int menuIndex = 0;
int submenuIndex = 0;

bool menuVisible = false;
bool inSubmenu = false;

// Screen modes
const char* mainMenu[] = {"Palette", "Text Temp." , "Pause"};
const int mainMenuSize = sizeof(mainMenu)/ sizeof(mainMenu[0]);

const char* colourModes[] = {"White-hot", "Black-hot", "Red-hot", "Red/blue", "Rainbow"};
const int colourModesSizes = sizeof(colourModes)/ sizeof(colourModes[0]);

int currentColourMode = 0;

const char* tempModes[] = {"Clear", "Min/Max/Avg", "Center temp.", "All info"};
const int tempModesSizes = sizeof(tempModes)/ sizeof(tempModes[0]);

int currentTempMode = 0;

bool isPaused = false;

// Buttons
bool btnPressed_ok = true;
bool btnPressed_left = true;
bool btnPressed_right = true;

int lastButtonState_ok = HIGH;
int lastButtonState_left= HIGH;
int lastButtonState_right = HIGH;
int buttonState;

void setup() {
  // Prepare buttons
  pinMode (btn_ok, INPUT);
  pinMode (btn_left, INPUT);
  pinMode (btn_right, INPUT);
  pinMode (led, OUTPUT);

  digitalWrite (led, LOW);

  // Initialize display
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(2);
  tft.fillScreen(ST7735_BLACK);
  tft.setTextColor(ST7735_WHITE);
  tft.setTextSize(2);
  tft.setCursor(24, 30);
  tft.println("Welcome");

  // Check if sensor is connected
  if (!amg.begin(0x68)) {
    tft.fillScreen(ST7735_BLACK);
    tft.setTextColor(ST7735_RED);
    tft.setTextSize(1);
    tft.setCursor(10, 10);
    tft.println("AMG8833 not found!");
    while (true);
  }
  delay (2000);                                         // Delay before program start
}

void loop() {

  // Create variables to handle button states
  int reading_ok = digitalRead(btn_ok);
  int reading_left = digitalRead(btn_left);
  int reading_right = digitalRead(btn_right);
  
  unsigned long currentMillis = millis();

  // Update debounce and add delay to auto-close menu
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

  // Debounce check
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading_ok == LOW && !btnPressed_ok) {          // OK button pressed check
      digitalWrite(led,HIGH);
      btnPressed_ok = true;  				                    // Mark button as pressed
      
      if (!menuVisible){                                // Show menu if not visible
        menuVisible = true; 
        drawMenu();
      } else if (!inSubmenu){                           // If in main menu, start from first submenu
        submenuIndex = 0;
        if(menuIndex == 2){                             // Toggle pause mode
          isPaused = !isPaused;
          inSubmenu = false;
          drawMenu();
        }else {                                         // Enter submenu
        inSubmenu = true;
        drawSubmenu ();
        }
      } else {
        if (menuIndex == 0){                            // Submenu switching (color palette)
          currentColourMode = submenuIndex;
          inSubmenu = false;
          drawMenu();
        }else if(menuIndex == 1){                       // Selecting temperature info mode and clearing bottom section
          currentTempMode = submenuIndex;
          tft.fillRect(74, 128, 128, 32, ST7735_BLACK);
          tft.fillRect(0, 128, 128, 32, ST7735_BLACK);
          inSubmenu = false;
          drawMenu();
        }
      }

    }

    if (currentMillis - del_menu >= 5000){               // 5-second inactivity clears display
      tft.fillRect(0, 128, 74, 8, ST7735_BLACK);
    }
    digitalWrite(led,LOW);
  }
  
  lastButtonState_ok = reading_ok;                       // Update OK button state

  if ((millis() - lastDebounceTime) > debounceDelay) {   // LEFT button debounce check
    if (reading_left == LOW && !btnPressed_left) {
      btnPressed_left = true;
      digitalWrite(led,HIGH);
      
      if (menuVisible){
        if (!inSubmenu){                                 // Main menu navigation left
          menuIndex = (menuIndex - 1 + mainMenuSize) % mainMenuSize;
          drawMenu();
        }else {                                          // Submenu navigation left
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

    if (currentMillis - del_menu >= 5000 && menuVisible){ // 5-second inactivity clears display
      tft.fillRect(0, 128, 74, 8, ST7735_BLACK);
      menuVisible = false;
      inSubmenu = false;
    }

    digitalWrite(led,LOW);
  }

  lastButtonState_left = reading_left;                    // Update LEFT button state

  if ((millis() - lastDebounceTime) > debounceDelay) {    // RIGHT button debounce check
    if (reading_right == LOW && !btnPressed_right) {
      btnPressed_right = true;
      digitalWrite(led,HIGH);

      if (menuVisible){
        if (!inSubmenu){                                  // Main menu navigation right
          menuIndex = (menuIndex + 1) % mainMenuSize;
          drawMenu();
        }else {                                           // Submenu navigation right
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

    if (currentMillis - del_menu >= 5000 && menuVisible){ // 5-second inactivity clears display
      tft.fillRect(0, 128, 74, 8, ST7735_BLACK);
      menuVisible = false;
      inSubmenu = false;
    }
    
    digitalWrite(led,LOW);
  }

  lastButtonState_right = reading_right;                  // Update RIGHT button state

  if (!isPaused && currentMillis - lastUpdate >= updateInterval) { 
    lastUpdate = currentMillis;                           // Update every 100 ms

    amg.readPixels(pixels);                               // Read temperature array from sensor

    // Copy data to array for sorting
    float sortedPixels [64];
    memcpy(sortedPixels, pixels, sizeof(pixels));

    bubbleSort(sortedPixels, 64);                         // Call function and pass data

    // Determine quantiles
    float qLow = getQuantile(sortedPixels, 64, 0.2);
    float qHigh = getQuantile(sortedPixels, 64, 0.95);

    // Raw sensor temperature data
    float rawMinT = 999;
    float rawMaxT = -999;
    float sumT = 0;

    float centerT = pixels[4 * gridSize + 4];             // Center pixel

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
    float range = qHigh - qLow;

    float desiredMin = 0;
    float desiredMax = 0;

    // Calculate desired temperature range for display
    if (range < minWindowSize){
      desiredMin = antiNoise + avgT - minWindowSize / 2.0;
      desiredMax = avgT + minWindowSize / 2.0;
    }else if (range > maxWindowSize){
      desiredMin = qHigh - maxWindowSize;
      desiredMax = qHigh;
    }else {
      desiredMin = qLow;
      desiredMax = qHigh;
    }

    // Smooth update of temperature limits
    tMin = 0.9 * tMin + 0.1 * desiredMin;
    tMax = 0.9 * tMax + 0.1 * desiredMax;

    // Draw temperature map to the screen
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
    
    drawTemp(rawMinT, rawMaxT, avgT, centerT);            // Call function to show temperature values

  }

  // Reset button state
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

// Sort temp. array
void bubbleSort (float arr[], int n) {
  for (int i = 0; i < n-1; i++){
    for (int j = 0; j < n-i-1; j++){
      if (arr[j] > arr[j+1]){
        float temp = arr[j];
        arr[j] = arr[j+1];
        arr[j+1] = temp;
      }
    }
  }
}

// Choising and returning quantile  
float getQuantile (float arr[],int n, float q){
  int index = (int)(q*(n-1));
  return arr[index];
}

// Data processing and transmission to the display as colour pallette
uint16_t mapTemperatureToColor(float temp, float tMin, float tMax) {

  // Range limitation
  if (temp < tMin) temp = tMin;
  if (temp > tMax) temp = tMax;

  int value = map(temp * 100, tMin * 100, tMax * 100, 0, 255);

  switch(currentColourMode) {
    case 0:
      return tft.color565(value, value, value); // Red – hot
      break;
    case 1:
      return tft.color565(255 - value, 255 - value, 255 - value); // White – hot
      break; 
    case 2:
      return tft.color565(value, value/4, 0); //  Red – hot, black – cold
      break;
    case 3:
      return tft.color565(value, 0, 255 - value); // Red – hot, blue – cold
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
    // Clear: empty dysplay
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
