// LoRa RX test: file is in
// p398dlp_fa2021_tests/group2_ventilator/LoRa_test_basestation_RX

// I am using the cda_cow_v02 circuit as a base station receiver.

// Choose which kind of processor from the following defines:
#define IS_MEGA         false
#define IS_ADALOGGER_M0 false
#define IS_LORA_M0      true
#define IS_GENERIC_M4   false 
#define IS_GROUP1_M4    false 

#define cda_cow true

#include <SPI.h>
#include <RH_RF95.h>
#include <LiquidCrystal.h>     

/*
// from Adafruit 
// https://learn.adafruit.com/adafruit-rfm69hcw-and-rfm96-rfm95-rfm98-lora-packet-padio-breakouts/rfm9x-test

Note that pin 2 is not available on an Adalogger. Also, pin 4 is the CS for the
built-in SD device.
*/

// define which pins do what! On the M0 and M4 pin 15 is also A1. pin 19 is A5.
#if IS_MEGA
#define RFM95_RST 2
#define RFM95_CS 4
#define RFM95_INT 3

#elif IS_ADALOGGER_M0
// CS is A5/19
#define RFM95_CS 19
// no reset line, so pick a non-existent pin.
#define RFM95_RST 29
// INT is A0/14
#define RFM95_INT 14
#define LED 16

#elif IS_LORA_M0
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3
#define HAS_LCD

#if cda_cow
const int rs = 12, en = 11, d4 = 5, d5 = 6, d6 = 9, d7 = 10;
#else
const int rs = 18, en = 11, d4 = 5, d5 = 6, d6 = 9, d7 = 19;
#endif

#elif IS_GENERIC_M4
#define RFM95_CS 10
#define RFM95_RST 9
#define RFM95_INT 15

#elif IS_GROUP1_M4
#define RFM95_CS 19
#define RFM95_RST 9
#define RFM95_INT 14
#define HAS_LCD

#endif

#if IS_LORA_M0
#define LED 12
#endif

// If you change to 434.0 or other frequency, must match TX's freq!
#define RF95_FREQ 915.0
// #define RF95_FREQ 868.0

// Singleton instance of the radio driver. Arguments like so:
// RH_RF95::RH_RF95(uint8_t slaveSelectPin, uint8_t interruptPin, RHGenericSPI& spi)
RH_RF95 rf95(RFM95_CS, RFM95_INT);

//////////////////////////// LCD stuff //////////////////////

// initialize the LCD library by associating any needed LCD interface pins
// with the pin numbers to which they are connected
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

void setup() 
{
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);
 
  // light up the serial monitor and wait for it to be ready.
  uint32_t t1234 = millis();
  Serial.begin(115200);
  while (!Serial && millis() - t1234 < 5000) {}

#if IS_MEGA2
  Serial.println("\n\n***** LoRa receive (RX) Test2 for Aruino MEGA 2560! *****");
#elif IS_ADALOGGER_M0
  Serial.println("\n\n***** LoRa receive (RX) Test2 for Adalogger M0! *****");
#elif IS_LORA_M0
  Serial.println("\n\n***** LoRa receive (RX) Test2 for Radio Feather M0! *****");
#elif IS_GENERIC_M4
  Serial.println("\n\n***** LoRa receive (RX) Test2 for Adafruit M4! *****");
#elif IS_GROUP1_M4
  Serial.println("\n\n***** LoRa receive (RX) Test2 for Group1 M4! *****");
#endif

  // manual reset: pull LOW to reset, put HIGH to allow LoRa to  function. 

#ifndef HAS_LCD
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);
#endif

#if IS_LORA_M0
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);
#endif

#ifdef HAS_LCD 
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  delay(100);

  // Print a message to the LCD.
  lcd.setCursor(0, 0);
  //         0123456789012345
  lcd.print("Now do LoRa test");
  lcd.setCursor(0, 1);
  lcd.print("recption. Waitng");
#endif

  while (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    while (1);
  }
  Serial.println("LoRa radio init OK!");

  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1);
  }
  Serial.print("Just set Freq to: "); Serial.println(RF95_FREQ);
  
  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then 
  // you can set transmitter powers from 5 to 23 dBm:
  rf95.setTxPower(23, false);
  Serial.println("Just set power.");

  Serial.println("Now listening for a transmission...");

}
  
void loop()
{
  if (rf95.available())
  {
    ////////////// receive a message //////////////

    // Should be a message for us now   
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    
    if (rf95.recv(buf, &len))
    {
      digitalWrite(LED, HIGH);

#if IS_MEGA
      RH_RF95::printBuffer("\nArduino Mega 2560 (LoRa_test_basestation_RX) received: ", buf, len);
#elif IS_ADALOGGER_M0
      RH_RF95::printBuffer("\nAdalogger Feather M0 (LoRa_test_basestation_RX) received: ", buf, len);
#elif IS_LORA_M0
      RH_RF95::printBuffer("\nRadio Feather M0 (LoRa_test_basestation_RX) received: ", buf, len);
#elif IS_GENERIC_M4
      RH_RF95::printBuffer("\nAdafruit M4 (LoRa_test_basestation_RX) received: ", buf, len);
#elif IS_GROUP1_M4
      RH_RF95::printBuffer("\nGroup1 M4 (LoRa_test_basestation_RX) received: ", buf, len);
#endif

      Serial.print("base station frequency (MHz): ");
      Serial.println(RF95_FREQ);
      Serial.print("The message, as characters, is '");
      Serial.print((char*)buf);
      Serial.println("'");
      Serial.print("RSSI (signal strength, in dB): ");
      Serial.println(rf95.lastRssi(), DEC);
      
#ifdef HAS_LCD 
      lcd.setCursor(0, 0);
      lcd.print("Received packet ");
      lcd.setCursor(0, 1);
      
      if(len > 16)
      {
        lcd.print("...             ");
        lcd.setCursor(3, 1);
        for(int ijk = 16; ijk > 0; ijk--)
        if(buf[len - ijk] >= 0x30 && buf[len - ijk] <= 0x39) {lcd.print((char)buf[len - ijk]);}  

      } else {      
        lcd.print((char*)buf);

      }

#endif
      ////////////// send a reply-to message //////////////

#if IS_MEGA
      char radiopacket[20] = "Mega 2560 RX ack   ";
      Serial.print("Mega 2560 will send the following reply: '"); Serial.print(radiopacket);
#elif IS_ADALOGGER_M0
      char radiopacket[20] = "Adalogger M0 RX ack";
      Serial.print("Adalogger M0 will send the following reply: '"); Serial.print(radiopacket);
#elif IS_LORA_M0
      char radiopacket[20] = "LoRa M0 RX ack";
      Serial.print("LoRa M0 will send the following reply: '"); Serial.print(radiopacket);
#elif IS_GENERIC_M4
      char radiopacket[20] = "Feather M4 RX ack  ";
      Serial.print("Feather M4 will send the following reply: '"); Serial.print(radiopacket);
#elif IS_GROUP1_M4
      char radiopacket[20] = "Group1 M4 RX ack   ";
      Serial.print("Group1 M4 will send the following reply: '"); Serial.print(radiopacket);
#endif
      Serial.println("'");
      radiopacket[19] = 0;
      
      Serial.println("LoRa_test_basestation_RX now sending packet..."); delay(10);
      rf95.send((uint8_t *)radiopacket, 20);
      
      Serial.println("Waiting for packet to complete..."); delay(10);
      rf95.waitPacketSent();
      Serial.println("Reply-to packet sent.");

#ifdef HAS_LCD 
      lcd.setCursor(0, 0);
      lcd.print("ACK sent. We got");
      //         0123456789012345 
#endif

    }
    else
    {
      Serial.println("Receive failed, even though rf95 was available");
    }
  }
}
