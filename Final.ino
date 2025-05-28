#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>
#include <BH1750.h>
#include <array>

// //BME680 Setup
// Adafruit_BME680 bme;

// //BH1750 Setup
// BH1750 lightMeter(0x23);

//SDI-12 Setup
#define DIRO 7
std::array<float, 5> get_data();
String command;
char deviceAddress = '0';
String deviceIdentification = "ENG20009";

void setup() {
  //Arduino IDE Serial Monitor
  Serial.begin(9600);
  sensor_setup();
  // ================ BME680 ================
  // if (!bme.begin(0x76)) {
  //   Serial.println("Could not find a valid BME680 sensor, check wiring!");
  //   while (1);
  // }
    // Set the temperature, pressure and humidity oversampling
  // bme.setTemperatureOversampling(BME680_OS_8X);
  // bme.setPressureOversampling(BME680_OS_8X);
  // bme.setHumidityOversampling(BME680_OS_2X);


  // ================ BH1750 ================
  // Wire.begin();
  // lightMeter.begin();


  // ================ SDI-12 ================
  Serial1.begin(1200, SERIAL_7E1);  //SDI-12 UART, configures serial port for 7 data bits, even parity, and 1 stop bit
  pinMode(DIRO, OUTPUT);               //DIRO Pin

  //HIGH to Receive from SDI-12
  digitalWrite(DIRO, HIGH);
}

void loop() {
  sdi_loop();
  
}

void sdi_loop() {
  int byte;
  //Receive SDI-12 over UART and then print to Serial Monitor
  if(Serial1.available()) {
    byte = Serial1.read();        //Reads incoming communication in bytes
    //Serial.println(byte);
    if (byte == 33) {             //If byte is command terminator (!)
      SDI12Receive(command);
      command = "";               //reset command string
    } else {
      if (byte != 0) {            //do not add start bit (0)
      command += char(byte);      //append byte to command string
      }
    }
  }
}

float data[5] = {-1.0, -1.0, -1.0, -1.0, -1.0};
float time_spent;
int num_data = 0;
void SDI12Receive(String input) {
  Serial.println(input);
  if (input.charAt(0) == '?') {
    SDI12Send(String(deviceAddress)); // Return the device address
  } else if (input.charAt(0) == deviceAddress) { 
    // Check if this device matches the adderss being addressed

    if (input.charAt(1) == 'b') { 
      deviceAddress = input.charAt(2);
      SDI12Send(String(deviceAddress));
    }
    if (input.charAt(1) == 'M' || input.charAt(1) == 'R') { // Start measurement at position 
      int sensor = input.charAt(2) + 0;
      float time_start = millis();
      // Start and take the measurement and record it
      std::array<float, 5> new_data = get_data();
      num_data = 0;
      // Get the size of actual data in the array
      for (int i = 0; i < 5; i++) {
        if (new_data[i] != -1.0) {
          num_data++;
          data[i] = new_data[i];
        }
      }

      float time_end = millis();
      time_spent = time_end - time_start;
      String return_string = String(deviceAddress);
    } 
    if (input.charAt(1) == 'D' || input.charAt(1) == 'R') {
      int sensor = input.charAt(2) + 0;
      // Send back the current measurement
      // Return string in format 0+data[0]+data[1]+data[2]+data[3]+data[4]
      String return_string = String(deviceAddress);
      for (int i = 0; i < num_data; i++) {
        return_string += "+" + String(data[i]);
      }
      SDI12Send(return_string);
      // Send the return_string
    }
  }
}

void SDI12Send(String message) {
  digitalWrite(DIRO, LOW);
  delay(100);
  Serial1.print(message + String("\r\n"));
  Serial1.flush();    //wait for print to finish
  Serial1.end();
  Serial1.begin(1200, SERIAL_7E1);
  digitalWrite(DIRO, HIGH);
  //secondruntoken = 0;
}