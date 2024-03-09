#include "Wire.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_DPS310.h>
#include <SPI.h>
#include <SD.h>
#include <vector>

#define TCAADDR 0x70
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3
#define SD_CS_PIN 17

File myFile;
Adafruit_DPS310 dps;
Adafruit_Sensor *dps_pressure = dps.getPressureSensor();

String filename = "DPSDATA5.csv";

void tcaselect(uint8_t i) {
  if (i > 7) return;
 
  Wire.beginTransmission(TCAADDR);
  Wire.write(1 << i);
  Wire.endTransmission();  
}

float avg(float input[]) {
  float s = 0;
  for (int i = 0; i < sizeof(input); i++) {
    s += input[i];
  }
  return s/sizeof(input);
}

float stdev(float input[]) {
  float mean = avg(input);
  float ss = 0;
  for (int i = 0; i < sizeof(input); i++) {
    ss += pow((input[i] - mean),2);
  }
  return pow(ss/(sizeof(input)-1),.5);
}

void setup() {
  
  Serial.begin(115200);
  while (!Serial);
  Wire.begin();

  tcaselect(4);
  while(!dps.begin_I2C()) {}
  Serial.println("DPS OK!");

  // Disable radio
  pinMode(RFM95_CS, OUTPUT);
  digitalWrite(RFM95_CS, HIGH);
  //delay(100);
  pinMode(SD_CS_PIN, OUTPUT);
  
  // Set up SD
  Serial.print("Initializing SD card. . .");
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("Initialization failed!");
    while (1);
  }
  Serial.println("initialization done.");
  myFile = SD.open(filename, FILE_WRITE);
  if (myFile) {
    Serial.println("File opened successfully. . .");
  } else {
    Serial.println("Error opening file!");
    while(1) yield();
  }

  dps.configurePressure(DPS310_64HZ, DPS310_64SAMPLES);

  float ave;
  float std;

  int timeout = 0;

  while (!myFile) {
    Serial.println("Accessing file...");
    delay(1000);
    timeout += 1;
    if (timeout > 5) {
      Serial.println("Failed to access file");
      abort();
    }
  }

  Serial.println("How many sets?");
  while (!Serial.available()) {}
  int num_sets = Serial.parseInt();

  for (int s = 1; s <= num_sets; s++) {
    Serial.println("Enter elevation");
    while(!Serial.available()) {}
    float elevation = Serial.parseFloat();

    float measurements[100];
    myFile.print(elevation,3);
    myFile.println(" m");
  
    for (int n = 1; n <= 100; n++) {
      sensors_event_t temp_event, pressure_event;
      myFile.print(n);
      myFile.print(",");

      while (!dps.pressureAvailable()) {}
      dps_pressure->getEvent(&pressure_event);
      myFile.println(pressure_event.pressure,6);
      measurements[n-1] = pressure_event.pressure,6;
    }
  
    ave = avg(measurements);
    std = stdev(measurements);
    myFile.print("mu,");
    myFile.println(ave,6);
    myFile.print("std,");
    myFile.println(std,6);
    myFile.println();
  }

  myFile.close();
  Serial.println("Done!");
}

void loop() {}
