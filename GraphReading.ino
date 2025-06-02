#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <BH1750.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

// ========== TFT Display Setup ==========
#define TFT_CS    10
#define TFT_DC    7
#define TFT_RST   6
#define TFT_SCLK  13
#define TFT_MOSI  11

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

//Button Setup
#define BTN_UP     2
#define BTN_DOWN   3
#define BTN_SELECT 4
#define BTN_BACK   5

//Sensor Addresses
#define BME_ADDR1   0x76
#define BME_ADDR2   0x77
#define BH1750_ADDR1 0x23
#define BH1750_ADDR2 0x5C

Adafruit_BME280 bme280;
BH1750 lightMeter;

//Sensor Detection Flags
bool detectedBME280 = false;
bool detectedBH1750 = false;

//Graph Parameters
#define GRAPH_WIDTH   100
#define GRAPH_HEIGHT  80
#define GRAPH_X       20
#define GRAPH_Y       20
#define MAX_SENSOR    200  // Adjust based on sensor range

int graphData[GRAPH_WIDTH];
int currentSensorMode = 0;  // 0 = Temperature, 1 = Humidity, 2 = Pressure, 3 = Light

//Menu Setup
const int menuLength = 4;  
int menuIndex = 0;
String menuOptions[menuLength] = {
  "Temperature",
  "Humidity",
  "Pressure",
  "Light"
};

void setup() {
  Serial.begin(9600);
  Wire.begin();

  // Initialize TFT
  tft.initR(INITR_BLACKTAB);
  tft.fillScreen(ST77XX_BLACK);
  tft.setRotation(1);

  // Initialize buttons
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);
  pinMode(BTN_BACK, INPUT_PULLUP);

  // Detect sensors
  detectSensors();
  drawMenu();
}

void loop() {
  // Menu navigation
  if (digitalRead(BTN_UP) == LOW) {
    menuIndex = (menuIndex - 1 + menuLength) % menuLength;
    drawMenu();
    delay(200);
  }

  if (digitalRead(BTN_DOWN) == LOW) {
    menuIndex = (menuIndex + 1) % menuLength;
    drawMenu();
    delay(200);
  }

  if (digitalRead(BTN_SELECT) == LOW) {
    currentSensorMode = menuIndex;
    showGraph();
    delay(200);
  }

  if (digitalRead(BTN_BACK) == LOW) {
    drawMenu();
    delay(200);
  }
}

//Sensor Detection
void detectSensors() {
  // Try BME280 first
  if (bme280.begin(BME_ADDR1) || bme280.begin(BME_ADDR2)) {
    detectedBME280 = true;
    Serial.println("BME280 detected.");
  }

 
  if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, BH1750_ADDR1) || 
      lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, BH1750_ADDR2)) {
    detectedBH1750 = true;
    Serial.println("BH1750 detected.");
  }

  if (!detectedBME280 && !detectedBH1750) {
    Serial.println("No sensors detected!");
  }
}

//Menu Drawing
void drawMenu() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);

  int yOffset = 20;
  for (int i = 0; i < menuLength; i++) {
    if (i == menuIndex) {
      tft.setTextColor(ST77XX_GREEN);
      tft.setCursor(0, yOffset + i * 20);
      tft.print("> ");
    } else {
      tft.setTextColor(ST77XX_WHITE);
      tft.setCursor(0, yOffset + i * 20);
      tft.print("  ");
    }
    tft.print(menuOptions[i]);
  }
}

//Graph Display
void showGraph() {
  tft.fillScreen(ST77XX_WHITE);
  drawStaticElements();

  unsigned long lastUpdate = 0;

  while (digitalRead(BTN_BACK) == HIGH) {
    if (millis() - lastUpdate >= 1000) {
      lastUpdate = millis();

      float sensorValue = 0;
      String unit = "";

      // Read selected sensor
      switch (currentSensorMode) {
        case 0:  // Temperature
          if (detectedBME280) {
            sensorValue = bme280.readTemperature();
            unit = "°C";
          }
          break;
        case 1:  // Humidity
          if (detectedBME280) {
            sensorValue = bme280.readHumidity();
            unit = "%";
          }
          break;
        case 2:  // Pressure
          if (detectedBME280) {
            sensorValue = bme280.readPressure();// / 100.0F;
            unit = "hPa";
          }
          break;
        case 3:  // Light
          if (detectedBH1750) {
            sensorValue = lightMeter.readLightLevel();
            unit = "lux";
          }
          break;
      }

      // Shift data left (newest on right)
      for (int i = 0; i < GRAPH_WIDTH - 1; i++) {
        graphData[i] = graphData[i + 1];
      }
      graphData[GRAPH_WIDTH - 1] = (int)sensorValue;

      noFlickerRedraw(unit);
    }
  }

  drawMenu();  // Return to menu on BACK press
}

//Graph Rendering
void noFlickerRedraw(String unit) {
  tft.fillRect(GRAPH_X, GRAPH_Y, GRAPH_WIDTH, GRAPH_HEIGHT, ST77XX_WHITE);
  
  int prevX = GRAPH_X;
  int prevY = GRAPH_Y + GRAPH_HEIGHT - constrain(map(graphData[0], 0, MAX_SENSOR, 0, GRAPH_HEIGHT), 0, GRAPH_HEIGHT);
  for (int i = 1; i < GRAPH_WIDTH; i++) {
    int x = GRAPH_X + i;
    int y = GRAPH_Y + GRAPH_HEIGHT - constrain(map(graphData[i], 0, MAX_SENSOR, 0, GRAPH_HEIGHT), 0, GRAPH_HEIGHT);
    tft.drawLine(prevX, prevY, x, y, ST77XX_RED);
    prevX = x;
    prevY = y;
  }

  // Display current value
  tft.setTextColor(ST77XX_RED, ST77XX_WHITE);
  tft.setCursor(5, 5);
  tft.print("Value: ");
  tft.print(graphData[GRAPH_WIDTH - 1]);
  tft.print(" ");
  tft.print(unit);
  tft.setCursor(5, 15);
  tft.print("Mode: ");
  tft.print(menuOptions[currentSensorMode]);
}

//Static Graph Elements
void drawStaticElements() {
  // Axes (black)
  tft.drawFastHLine(GRAPH_X, GRAPH_Y + GRAPH_HEIGHT, GRAPH_WIDTH, ST77XX_BLACK);
  tft.drawFastVLine(GRAPH_X, GRAPH_Y, GRAPH_HEIGHT, ST77XX_BLACK);

  // Labels (red)
  tft.setTextColor(ST77XX_RED, ST77XX_WHITE);
  tft.setCursor(GRAPH_X + GRAPH_WIDTH/2 - 15, GRAPH_Y + GRAPH_HEIGHT + 15);
  tft.print("Time (s)");
  tft.setCursor(GRAPH_X + GRAPH_WIDTH + 5, GRAPH_Y);
  tft.print("Value");

  // Grid (cyan)
  for (int i = 0; i <= GRAPH_WIDTH; i += 20) {
    tft.drawFastVLine(GRAPH_X + i, GRAPH_Y, GRAPH_HEIGHT, ST77XX_CYAN);
  }
  for (int i = 0; i <= GRAPH_HEIGHT; i += 20) {
    tft.drawFastHLine(GRAPH_X, GRAPH_Y + i, GRAPH_WIDTH, ST77XX_CYAN);
  }
}
