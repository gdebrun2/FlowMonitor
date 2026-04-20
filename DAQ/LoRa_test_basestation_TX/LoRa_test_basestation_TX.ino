// LoRa TX test: LoRa_test_basestation_TX.ino

// Choose which kind of processor from the following defines:
#define IS_MEGA         false
#define IS_ADALOGGER_M0 false
#define IS_LORA_M0      true
#define IS_GENERIC_M4   false 
#define IS_GROUP1_M4    false 

#include <SPI.h>
#include <RH_RF95.h>
#include <LiquidCrystal.h>

/*
// from Adafruit 
// https://learn.adafruit.com/adafruit-rfm69hcw-and-rfm96-rfm95-rfm98-lora-packet-padio-breakouts/rfm9x-test

Note that pin 2 is not available on an Adalogger. Also, pin 4 is the CS for the
built-in SD device.
*/

// define which pins do what! On the M0 and M4 pin 15 is also A1.  pin 19 is A5.
#if IS_MEGA
#define RFM95_RST 2
#define RFM95_CS 4
#define RFM95_INT 3

#elif IS_ADALOGGER_M0
#define RFM95_CS 10
#define RFM95_RST 9
#define RFM95_INT 15

#elif IS_LORA_M0
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3
#define HAS_LCD

#elif IS_GENERIC_M4
#define RFM95_CS 10
#define RFM95_RST 9
#define RFM95_INT 15

#elif IS_GROUP1_M4
#define RFM95_CS 19
// noet that RST is not connecte to anything, but we need to define a pin (that is ignored)
#define RFM95_RST 9
#define RFM95_INT 14
// only the group 1 LoRa board has an LCD.
#define HAS_LCD

#endif

#if IS_LORA_M0
#define LED 12
#else
#define LED 13
#endif

// If you change to 434.0 or other frequency, must match RX's freq!
#define RF95_FREQ 915.0

// Singleton instance of the radio driver. Arguments like so:
// RH_RF95::RH_RF95(uint8_t slaveSelectPin, uint8_t interruptPin, RHGenericSPI& spi)
RH_RF95 rf95(RFM95_CS, RFM95_INT);

#if IS_LORA_M0
//////////////////////////// LCD stuff //////////////////////

// initialize the LCD library by associating any needed LCD interface pins
// with the pin numbers to which they are connected
const int rs = 18, en = 11, d4 = 5, d5 = 6, d6 = 9, d7 = 19;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
#endif

// packet counter, we increment per xmission
int16_t packetnum = 0;  

void setup() 
{
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);
 
 // light up the serial monitor and wait for it to be ready.
  Serial.begin(115200);
  while (!Serial);

#if IS_MEGA
  Serial.println("\n\n***** LoRa transmit (TX) Test2 for Arduino Mega 2560! *****");
#elif IS_ADALOGGER_M0
  Serial.println("\n\n***** LoRa transmit (TX) Test2 for Adalogger M0! *****");
#elif IS_LORA_M0
  Serial.println("\n\n***** LoRa transmit (TX) Test2 for Radio Feather M0! *****");
#elif IS_GENERIC_M4
  Serial.println("\n\n***** LoRa transmit (TX) Test2 for Adafruit M4! *****");
#elif IS_GROUP1_M4
  Serial.println("\n\n***** LoRa transmit (TX) Test2 for Adafruit M4! *****");
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
  lcd.print("transmission    ");
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

}
  
void loop()
{
  ////////////// send a message //////////////

#if IS_MEGA
  Serial.println("\n***** Mega 2560 sending to rf95 listener *****");
  char radiopacket[20] = "MEGA 2560 TX#      ";
#elif IS_ADALOGGER_M0
  Serial.println("\n***** Adalogger Feather M0 sending to rf95 listener *****");
  char radiopacket[20] = "AaloggerM0TX#      ";
#elif IS_LORA_M0
  Serial.println("\n***** Radio Feather M0 sending to rf95 listener *****");
  char radiopacket[20] = "RadioFthM0TX#      ";
#elif IS_GENERIC_M4
  Serial.println("\n***** Adafruit M4 sending to rf95 listener *****");
  char radiopacket[20] = "AdafruitM4TX#      ";
#elif IS_GROUP1_M4
  Serial.println("\n***** Group 1 M4 sending to rf95 listener *****");
  char radiopacket[20] = "Group1 M4 TX#      ";
#endif

  // Send a message to rf95_server
  itoa(packetnum++, radiopacket+13, 10);
  Serial.print("Sending the string '"); Serial.print(radiopacket);
  Serial.println("'");
  radiopacket[19] = 0;

#ifdef HAS_LCD 
  lcd.setCursor(0, 0);
  lcd.print("Sending packet  ");
  lcd.setCursor(0, 1);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print(packetnum);
#endif
  
  Serial.println("LoRa_test_basestation_TX sending now..."); delay(10);
  rf95.send((uint8_t *)radiopacket, 20);
  
  Serial.println("Waiting for packet to complete..."); delay(10);
  rf95.waitPacketSent();
  // Now wait for a reply
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);
  
  Serial.println("Packet sent. Wait for reply..."); 

#ifdef HAS_LCD 
  lcd.setCursor(0, 1);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  //         0123456789012345
  lcd.print("Packet sent     ");
#endif

  ////////////// receive a reply-to message //////////////

  // this is a workaround for an acknowledgement since waitAvailableTimeout
  // does not work reliably for me.


  uint32_t millis_now = millis();
  uint32_t time_out = 5000;

  while(!rf95.available() && (millis() - millis_now < time_out)){}

  if (rf95.available())
  {
    // Should be a reply message for us now   
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    
    if (rf95.recv(buf, &len))
    {
      digitalWrite(LED, HIGH);
      RH_RF95::printBuffer("Sender (LoRa_test_basestation_TX) has received reply: ", buf, len);
      Serial.print("That message, as characters, is '");
      Serial.print((char*)buf);
      Serial.println("'");
      Serial.print("RSSI (signal strength, in dB): ");
      Serial.println(rf95.lastRssi(), DEC);
      Serial.print("Time (ms) spent waiting for reply: ");
      Serial.println(millis() - millis_now);
#ifdef HAS_LCD 
      lcd.setCursor(0, 0);
      lcd.print("We have received");
      lcd.setCursor(0, 1);
      //         0123456789012345
      lcd.print("a reply.        ");
      lcd.setCursor(9, 1);
      lcd.print(packetnum);
#endif
    }
    else
    {
      Serial.println("Receive (as a reply-to) failed");
    }
  } else {
    Serial.println("Timed out waiting for reply.");
  }
  delay(1000);
}
