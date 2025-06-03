// SD card library
#include "SdFat.h"

// RTC module library
#include "RTClib.h"
#include <DFRobot_BMX160.h>
#include <string>

// Define file system type
SdFs sd;

// Define files for the four sensors
FsFile temperature;
FsFile humidity;
FsFile pressure;
FsFile light;

// Initialise RTC libraries
RTC_DS1307 rtc;
DFRobot_BMX160 bmx160;

// Set chip to SDCS pin A3 on board
const uint8_t SD_CS_PIN = A3;
//
// Define constants for pin numbers on LCD module related to SD card
const uint8_t SOFT_MISO_PIN = 12;
const uint8_t SOFT_MOSI_PIN = 11;
const uint8_t SOFT_SCK_PIN  = 13;

// SdFat software SPI template
SoftSpiDriver<SOFT_MISO_PIN, SOFT_MOSI_PIN, SOFT_SCK_PIN> softSpi;

#define SD_CONFIG SdSpiConfig(SD_CS_PIN, SHARED_SPI, SD_SCK_MHZ(0), &softSpi)

// RTC simplification of date using character array
char daysOfTheWeek[7][12] = {"Saturday", "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday"};

void setup() {
  Serial.begin(9600);
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

  // SaveSensorData arguments: 1-float value, 2-either "Temperature", "Humidity", "Pressure", or "Light"
  // ReadSD arguments: 1-Fsfile, 2-const char "temperature.txt", "humidity.txt", "pressure.txt", "light.txt"
  SaveSensorData(25, "Temperature");
  //SaveSensorData(27, "Temperature");
  //SaveSensorData(29, "Temperature");
  Serial.print(ReadSD(temperature, "temperature.txt"));

}

void loop() {

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