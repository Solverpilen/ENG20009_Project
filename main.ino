#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>
#include <BH1750.h>
#include <array>
#include <DueTimer.h>


// ?! -> Get Address
// aAb! change address a to address b
// aM! Take measurements
// aD! Return results
// aR! take measurement and return result
// with M, D and R add an integer after to get specific sensor readings

// //BME680 Setup
// Adafruit_BME680 bme;

// //BH1750 Setup
// BH1750 lightMeter(0x23);
#define BTN_SELECT 4

int TIMER_FREQ = 1; // 1s / timer_freq

//SDI-12 Setup

#define DIRO 7
std::array<float, 10> get_data();
String command;
char deviceAddress = '0';
String deviceIdentification = "ENG20009";

bool timer_running = false;

void setup() {
  //Arduino IDE Serial Monitor
  // Interrupts to just wreck everything idk why
  // Timer3.attachInterrupt(timer_inter);
  // attachInterrupt(BTN_SELECT, button_inter, RISING);
  Serial.begin(9600);
  sensor_setup();
  lcd_setup();

  
  
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

void timer_inter() {
  Serial.println("Check");
  SDI12Receive("0R!");
}

void button_inter() {
  SDI12Send("Menu Selected");
  Serial.println("Interupt?");
}

void loop() {
  sdi_loop();
  lcd_loop();
  
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

float data[10] = {-1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0 -1.0};
float time_spent;
int num_data = 0;



void SDI12Receive(String input) {
  int input_length = input.length();
  Serial.println("Recieved Input: " + input);
  if (input.charAt(0) == '?') {
    SDI12Send(String(deviceAddress)); // Return the device address
  } else if (String(input.charAt(0)) == String(deviceAddress)) { 
    // Check if this device matches the adderss being addressed
    if (input.charAt(1) == 'A') { 
      deviceAddress = input.charAt(2);
      SDI12Send(String(deviceAddress));
    }
    if (input.charAt(1) == 'M' || input.charAt(1) == 'R') { // Start measurement at position 
      float time_start = millis();
      int sensor = input.charAt(2) + 0;
      // Start and take the measurement and record it
      std::array<float, 10> new_data = get_data();
      num_data = 0;
      for (int i = 0; i < 5; i++) {
        if (new_data[i] != -1.0) {
          num_data++;
        }
      }
      if (input_length == 3) { // 3 means we were passed a specific address to get
        int index = String(input.charAt(2)).toInt();
        data[index] = new_data[index];
      } else {
        // Get the size of actual data in the array
        for (int i = 0; i < 5; i++) {
          if (new_data[i] != -1.0) {
            data[i] = new_data[i];
          }
        }
      }

      float time_end = millis();
      time_spent = time_end - time_start;
      String return_string = String(deviceAddress);
      return_string += String(time_spent);
      return_string += String(num_data);
      SDI12Send(return_string);
    } 
    if (input.charAt(1) == 'D' || input.charAt(1) == 'R') {
      int sensor = input.charAt(2) + 0;
      // Send back the current measurement
      // Return string in format 0+data[0]+data[1]+data[2]+data[3]+data[4]
      String return_string = String(deviceAddress);
      if (input_length == 3) {
          int index = String(input.charAt(2)).toInt();
          if (data[index] == -1.0) {
            SDI12Send("NO DATA @" + String(index));
            SDI12Send("FINAL DATA @" + String(num_data - 1));
          }
          else {
            return_string += "+" + String(data[index]);
          }
      } else {
        for (int i = 0; i < num_data; i++) {
          return_string += "+" + String(data[i]);
        }
        // Send the return_string
      }
      SDI12Send(return_string);
    }
    if (input.charAt(1) == 'T') {
      if (timer_running) {
        // Timer3.stop();
        timer_running = false;
        SDI12Send("STOP TIMED SEND");
      } else {
        Serial.println("Start TImer");
        // Timer3.start(10000);
        timer_running = true;
        SDI12Send("START TIMED SEND");
      }
      SDI12Send("TIMER does NOT work");
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