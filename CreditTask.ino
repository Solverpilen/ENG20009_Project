#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <BH1750.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <DFRobot_BMX160.h>
#include <SPI.h>
#include "SdFat.h"
#include "RTClib.h"

FsFile temperature;
FsFile humidity;
FsFile pressure;
FsFile light;

// ========== TFT Display Setup ==========
#define TFT_CS    10
#define TFT_DC    7
#define TFT_RST   6
#define TFT_SCLK  13
#define TFT_MOSI  11

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

// Button Setup
#define BTN_UP     2
#define BTN_DOWN   3  // This will be our save button
#define BTN_SELECT 4
#define BTN_BACK   5

// Sensor Addresses
#define BME_ADDR1   0x76
#define BME_ADDR2   0x77
#define BH1750_ADDR1 0x23
#define BH1750_ADDR2 0x5C

Adafruit_BME280 bme280;
BH1750 lightMeter;
RTC_DS1307 rtc;

// SD Card Setup
#define SD_CS_PIN A3
const uint8_t SOFT_MISO_PIN = 12;
const uint8_t SOFT_MOSI_PIN = 11;
const uint8_t SOFT_SCK_PIN = 13;
SoftSpiDriver<SOFT_MISO_PIN, SOFT_MOSI_PIN, SOFT_SCK_PIN> softSpi;
#define SD_CONFIG SdSpiConfig(SD_CS_PIN, SHARED_SPI, SD_SCK_MHZ(0), &softSpi)
SdFs sd;
FsFile dataFile;

char daysOfTheWeek[7][12] = {"Saturday", "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday"};

// Sensor Detection Flags
bool detectedBME280 = false;
bool detectedBH1750 = false;
bool sdInitialized = false;

// Graph Parameters
#define GRAPH_WIDTH   100
#define GRAPH_HEIGHT  80
#define GRAPH_X       20
#define GRAPH_Y       20
#define MAX_SENSOR    200  // Adjust based on sensor range

int graphData[GRAPH_WIDTH];
int currentSensorMode = 0;  // 0 = Temperature, 1 = Humidity, 2 = Pressure, 3 = Light

// Menu Setup
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
   pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);
  // Begin and adjust RTC
  rtc.begin();
  rtc.adjust(DateTime(2025, 6, 3, 1, 20, 0));

  // Check existence of SD card
  if (!sd.begin(SD_CONFIG)) {
    Serial.println("SD card initialization failed!");
    sd.initErrorHalt();
    while (1);
  }

  CreateFile(temperature, "temperature.txt");
  CreateFile(humidity, "humidity.txt");
  CreateFile(pressure, "pressure.txt");
  CreateFile(light, "light.txt");

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
    // Save button pressed - only works in graph mode
    if (currentSensorMode == 0 && detectedBME280) {
      SaveSensorData(bme280.readTemperature(), "Temperature");
      
    }
    else if (currentSensorMode == 1 && detectedBME280) {
      SaveSensorData(bme280.readHumidity(), "Humidity");
      Serial.println(bme280.readHumidity());
      
    } else if (currentSensorMode == 2 && detectedBME280) {
      SaveSensorData(bme280.readPressure(), "Pressure");
      Serial.println(bme280.readPressure());
     
    } else if (currentSensorMode == 3 && detectedBH1750) {
      SaveSensorData(lightMeter.readLightLevel(), "Light");
      
    }
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
void SaveSensorData(float sensorData, const char* sensorType) {

  // Depending on the sensor being graphed at the moment, save to relevant text file within SD card
  if (sensorType == "Temperature") {
      WriteSD(temperature, "temperature.txt", sensorData);

  } else if (sensorType == "Humidity") {
      WriteSD(humidity, "humidity.txt", sensorData);

  } else if (sensorType == "Pressure") {
      WriteSD(pressure, "pressure.txt", sensorData);

  } else if (sensorType == "Light") {
      WriteSD(light, "light.txt", sensorData);

  }
}

void CreateFile(File sensor_type, char* file_name) {

  Serial.print(file_name);
  Serial.println(" has been created.");

  // Open/create a file for writing
  if (!sensor_type.open(file_name, O_RDWR | O_CREAT)) {
    Serial.print(file_name);
    sd.errorHalt(F("File failed to open."));
  }
}

void WriteSD(File sensor_type, char* file_name, float sensorData) {
  DateTime present = rtc.now();
  Serial.print("---Saving To ");
  Serial.print(file_name);
  Serial.println("---");
  sensor_type.open(file_name, O_RDWR);

  // Return to line 0 within file
  sensor_type.seekEnd(0);

  // Save current line in sensor
  sensor_type.print(sensorData);
  sensor_type.print(", ");
  sensor_type.print(present.hour(), DEC);
  sensor_type.print(':');
  sensor_type.print(present.minute(), DEC);
  sensor_type.print(':');
  sensor_type.print(present.second(), DEC);
  sensor_type.print(" [");
  sensor_type.print(daysOfTheWeek[present.dayOfTheWeek()]);
  sensor_type.print("] ");
  sensor_type.print(present.day(), DEC);
  sensor_type.print('/');
  sensor_type.print(present.month(), DEC);
  sensor_type.print('/');
  sensor_type.println(present.year(), DEC);

  sensor_type.close();
}

String ReadSD(File sensor_type, char* file_name) {
  Serial.print("---Reading from ");
  Serial.print(file_name);
  Serial.println("---");
  
  sensor_type.open(file_name, O_RDWR);
  sensor_type.seek(0);
  String contents = sensor_type.readString();
  sensor_type.close();

  return contents;
}



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
            unit = "Celcius";
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
            sensorValue = bme280.readPressure();
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
