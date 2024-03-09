#include "Wire.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_DPS310.h>
#include <SPI.h>
#include <SD.h>

#define TCAADDR 0x70
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3
#define SD_CS_PIN 17

File myFile;
Adafruit_DPS310 dps0; 
Adafruit_DPS310 dps1; 
Adafruit_DPS310 dps2; 
Adafruit_DPS310 dps3;
Adafruit_DPS310 dps4; 
Adafruit_DPS310 dps5; 
Adafruit_DPS310 dps6; 
Adafruit_DPS310 dps7;
Adafruit_DPS310 dps8;


Adafruit_Sensor *dps_pressure_0 = dps0.getPressureSensor();
//Adafruit_Sensor *dps_temp_0 = dps0.getTemperatureSensor();
Adafruit_Sensor *dps_pressure_1 = dps1.getPressureSensor();
Adafruit_Sensor *dps_pressure_2 = dps2.getPressureSensor();
Adafruit_Sensor *dps_pressure_3 = dps3.getPressureSensor();
Adafruit_Sensor *dps_pressure_4 = dps4.getPressureSensor();
Adafruit_Sensor *dps_pressure_5 = dps5.getPressureSensor();
Adafruit_Sensor *dps_pressure_6 = dps6.getPressureSensor();
Adafruit_Sensor *dps_pressure_7 = dps7.getPressureSensor();
Adafruit_Sensor *dps_pressure_8 = dps8.getPressureSensor();

String filename = "DATA.txt";


void tcaselect(uint8_t i) {
  if (i > 7) return;
 
  Wire.beginTransmission(TCAADDR);
  Wire.write(1 << i);
  Wire.endTransmission();  
}



void setup() {
  
  Serial.begin(115200);
  while (!Serial);
  Wire.begin();
  
 
  Serial.print("56");

  while(!dps8.begin_I2C(0x76)) {Serial.println("Connecting to 8"); delay(1000);}
  Serial.println("8 is good");
 

  // Disable radio
  pinMode(RFM95_CS, OUTPUT);
  digitalWrite(RFM95_CS, HIGH);
  pinMode(SD_CS_PIN, OUTPUT);
  
 
  // Set up SD
  
  Serial.println("Initializing SD card. . .");
  while (!SD.begin(SD_CS_PIN)) {
    Serial.println("Initialization failed!");
    delay(1000);
  }
  Serial.println("initialization done.");
  
  myFile = SD.open(filename, FILE_WRITE);
  while (!myFile) {
    Serial.println("Error opening file!");
    delay(1000);
    
  } 
  Serial.println("File opened successfully. . .");

  if (myFile.size() > 10) {
    myFile.println("");
    myFile.println("");
  }


  // Setup highest precision


  tcaselect(0);
  dps0.configurePressure(DPS310_64HZ, DPS310_64SAMPLES);
  
  tcaselect(1);  
  dps1.configurePressure(DPS310_64HZ, DPS310_64SAMPLES);
  
  tcaselect(2);
  dps2.configurePressure(DPS310_64HZ, DPS310_64SAMPLES);
  
  tcaselect(3);
  dps3.configurePressure(DPS310_64HZ, DPS310_64SAMPLES);
  
  tcaselect(4);
  dps4.configurePressure(DPS310_64HZ, DPS310_64SAMPLES);
  
  tcaselect(5);
  dps5.configurePressure(DPS310_64HZ, DPS310_64SAMPLES);
    
  tcaselect(6);
  dps6.configurePressure(DPS310_64HZ, DPS310_64SAMPLES);
  
  tcaselect(7);
  dps7.configurePressure(DPS310_64HZ, DPS310_64SAMPLES);

  dps8.configurePressure(DPS310_64HZ, DPS310_64SAMPLES);
  
  
  int num_samples = 10;
  Serial.println("Input the flowrate");
  while (!Serial.available()) {};
  float flow_rate = Serial.parseFloat();
  myFile.println(flow_rate);
      
  for (int n = 1; n <= num_samples; n++) {

    sensors_event_t temp_event, pressure_event;

      
    myFile.print(n);
    myFile.print(",");

   
    tcaselect(0);
    while (!dps0.pressureAvailable()) {}
    dps_pressure_0->getEvent(&pressure_event);
    myFile.print(pressure_event.pressure,6);
    myFile.print(", ");
   

    tcaselect(1);
    while (!dps1.pressureAvailable()) {}
    dps_pressure_1->getEvent(&pressure_event);
    myFile.print(pressure_event.pressure,6);
    myFile.print(", ");

    tcaselect(2);
    while (!dps2.pressureAvailable()) {}
    dps_pressure_2->getEvent(&pressure_event);
    myFile.print(pressure_event.pressure,6);
    myFile.print(", ");

    tcaselect(3);
    while (!dps3.pressureAvailable()) {}
    dps_pressure_3->getEvent(&pressure_event);
    myFile.print(pressure_event.pressure,6);
    myFile.print(", ");

    tcaselect(4);
    while (!dps4.pressureAvailable()) {}
    dps_pressure_4->getEvent(&pressure_event);
    myFile.print(pressure_event.pressure,6);
    myFile.print(", ");

    tcaselect(5);
    while (!dps5.pressureAvailable()) {}
    dps_pressure_5->getEvent(&pressure_event);
    myFile.print(pressure_event.pressure,6);
    myFile.print(", ");

    tcaselect(6);
    while (!dps6.pressureAvailable()) {}
    dps_pressure_6->getEvent(&pressure_event);
    myFile.print(pressure_event.pressure,6);
    myFile.print(", ");

    tcaselect(7);
    while (!dps7.pressureAvailable()) {}
    dps_pressure_7->getEvent(&pressure_event);
    myFile.print(pressure_event.pressure,6);
    myFile.print(", ");

   
    while (!dps8.pressureAvailable()) {}
    dps_pressure_8->getEvent(&pressure_event);
    myFile.print(pressure_event.pressure,6);
    myFile.println("");
   


  }
  
  myFile.close();
  Serial.println("Done!");
  
}

  
void loop() {}
