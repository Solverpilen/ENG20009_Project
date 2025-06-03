#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <BH1750.h>
#include <array>

// // Define possible addresses
// #define BME_ADDR1 0x76
// #define BME_ADDR2 0x77
// #define BH1750_ADDR1 0x23
// #define BH1750_ADDR2 0x5C


void sensor_setup() {
  Serial.begin(9600);
  Wire.begin();
  delay(100);

  // Try to detect BME280 first
  if (bme280.begin(BME_ADDR1)) {
    Serial.println("Detected BME280 at 0x76");
    detectedBME280 = true;
  } else if (bme280.begin(BME_ADDR2)) {
    Serial.println("Detected BME280 at 0x77");
    detectedBME280 = true;
  }

  // Try to detect BH1750 only if BME280 not found
  if (!detectedBME280) {
    if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, BH1750_ADDR1)) {
      Serial.println("Detected BH1750 at 0x23");
      detectedBH1750 = true;
    } else if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, BH1750_ADDR2)) {
      Serial.println("Detected BH1750 at 0x5C");
      detectedBH1750 = true;
    }
  }

  if (!detectedBME280 && !detectedBH1750) {
    Serial.println("No BME280 or BH1750 detected.");
  }
}

std::array<float, 3> return_bme_data() {
  if(detectedBME280) {
    return std::array<float, 3> {bme280.readTemperature(), bme280.readHumidity(), bme280.readPressure()};
  }
}

float return_bh1_data() {
  if(detectedBH1750){
    return lightMeter.readLightLevel();
  }
}

std::array<float, 10> get_data() { 
  if (detectedBME280) {
    float temp = bme280.readTemperature();
    float hum = bme280.readHumidity();
    float pres = bme280.readPressure() / 100.0F;
    // Serial.print("Temp: "); Serial.print(temp); Serial.print(" °C, ");
    // Serial.print("Humidity: "); Serial.print(hum); Serial.print(" %, ");
    // Serial.print("Pressure: "); Serial.print(pres); Serial.println(" hPa");
    return {temp, hum, pres, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0};  // Return all BME280 data
  }
  else if (detectedBH1750) {
    float lux = lightMeter.readLightLevel();
    // Serial.print("Light: "); Serial.print(lux); Serial.println(" lx");
    return {lux, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0}; // Return lux
  }
  
  delay(1000);
  return {-1.0, -1.0, -1.0, -1.0, -1.0};
}


