// start doing simple things with Adafruit Feather M0 with RFM95 LoRa
// radio. I will use it as a 915 MHz device, and have attached a 3 inch 
// (quarter wavelength) whip antenna to the board.

// this file is radio_feather_test_send_receive.ino
//
// it makes a radio feather transmit and/or receive stuff. run it with a second 
// feather acting as the receiver/transmitter.

// user is queried from the serial monitor window whether the device executing the
// program will function as a base station (primarily as a receiver) or as a
// field-deployed device.

// Get some goodies from GitHub.
// https://github.com/cavemoa/Feather-M0-Adalogger/blob/master/DisplayZeroRegs/DisplayZeroRegs.ino

// I follow the instructions on the Adafruit site at 
// https://learn.adafruit.com/adafruit-feather-m0-radio-with-lora-radio-module/using-the-rfm-9x-radio

// download RadioHead-1.62.zip, put it in /Arduino_libraries_GDG then go to 
// sketch -> include library -> add .zip library and then go to 
// sketch -> include library -> radiohead. 
// Here's what moves into this file from doing this:
#include <SPI.h>
#include <RH_RF95.h>
#include <RH_NRF24.h>
#include <RH_Serial.h>
#include <RH_NRF51.h>
#include <radio_config_Si4460.h>
#include <RH_RF69.h>
#include <RHSoftwareSPI.h>
#include <RH_TCP.h>
#include <RH_MRF89.h>
#include <RHMesh.h>
#include <RH_RF24.h>
#include <RHGenericDriver.h>
#include <RHCRC.h>
#include <RHHardwareSPI.h>
#include <RHGenericSPI.h>
#include <RH_ASK.h>
#include <RHTcpProtocol.h>
#include <RH_NRF905.h>
#include <RadioHead.h>
#include <RH_CC110.h>
#include <RHDatagram.h>
#include <RHSPIDriver.h>
#include <RH_RF95.h>
#include <RHReliableDatagram.h>
#include <RHRouter.h>
#include <RHNRFSPIDriver.h>
#include <RH_RF22.h>

// whew, lots of include statements. 

///////////////////////////////////////////////////////

// now get the sample code from the Adafruit site. Here are Adafruit's comments 
// and compiler definitions:

// Feather9x_TX
// -*- mode: C++ -*-
// Example sketch showing how to create a simple messaging client (transmitter)
// with the RH_RF95 class. RH_RF95 class does not provide for addressing or
// reliability, so you should only use RH_RF95 if you do not need the higher
// level messaging abilities.
// It is designed to work with the other example Feather9x_RX

/* for feather32u4 
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 7
*/

// we are using a feather m0...

// for feather m0  

// "chip select" pin for the radio, used by the Atmel processor to control radio
#define RFM95_CS 8
// "reset" pin for the radio. Reset when this pin is pulled low.
#define RFM95_RST 4
// I am not sure what this does. (An interrupt pin??)
#define RFM95_INT 3
//

/* for shield 
#define RFM95_CS 10
#define RFM95_RST 9
#define RFM95_INT 7
*/

/* Feather 32u4 w/wing
#define RFM95_RST     11   // "A"
#define RFM95_CS      10   // "B"
#define RFM95_INT     2    // "SDA" (only SDA/SCL/RX/TX have IRQ!)
*/

/* Feather m0 w/wing 
#define RFM95_RST     11   // "A"
#define RFM95_CS      10   // "B"
#define RFM95_INT     6    // "D"
*/

#if defined(ESP8266)
  /* for ESP w/featherwing */ 
  #define RFM95_CS  2    // "E"
  #define RFM95_RST 16   // "D"
  #define RFM95_INT 15   // "B"

#elif defined(ESP32)  
  /* ESP32 feather w/wing */
  #define RFM95_RST     27   // "A"
  #define RFM95_CS      33   // "B"
  #define RFM95_INT     12   //  next to A

#elif defined(NRF52)  
  /* nRF52832 feather w/wing */
  #define RFM95_RST     7   // "A"
  #define RFM95_CS      11   // "B"
  #define RFM95_INT     31   // "C"
  
#elif defined(TEENSYDUINO)
  /* Teensy 3.x w/wing */
  #define RFM95_RST     9   // "A"
  #define RFM95_CS      10   // "B"
  #define RFM95_INT     4    // "C"
#endif

// If you want to change the frequency to 434.0 (or something else)
// do it here. This must match the other radio device's freqiency, of course.

#define RF95_FREQ 915.0

// Instantiate a "singleton instance" of the radio driver.
RH_RF95 rf95(RFM95_CS, RFM95_INT);

///////////////////////////////////////////////////////

// now define and include more stuff.

// red LED is attached here:
#define red_LED_pin     13 

// flip the state of the red LED each time we write through the radio
bool LED_toggle;

// packet counter, we increment per transmission
int16_t packetnum = 0;  

// is this processor an initiator or a responder?
bool I_am_initiator;

///////////////////////////////////////////////////////
// setup
///////////////////////////////////////////////////////

// the setup function executes once.

void setup() {

  // open the serial line so we can talk to the serial monitor window
  Serial.begin(115200);

  // wait for the serial port to be ready
  while (!Serial) {delay(1);}
  
  // initialize digital pin connected to the LED as an output.
  pinMode(red_LED_pin, OUTPUT);

  // initialize the LED state switch
  LED_toggle = true;

  // It is tempting to think that RFM95 stands for "Radio Feather 
  // M0 something or other, but it's actually from the transceiver module 
  // part number: RFM95W/96W/98W.

  // RFM95_RST is used by the processor as a reset pin for the radio.
  // I think it's actually a reset-bar line: rest when pulled low.
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  // make very sure the reset line has time to settle.
  delay(100);
  
  Serial.println("Feather LoRa TX/RX Test!");

  // manual reset, with 10 millisecond dwell times in each state
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  while (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    Serial.println("Uncomment '#define SERIAL_DEBUG' in RH_RF95.cpp for detailed debug info");
    while (1);
  }

  Serial.println("LoRa radio init OK!");

  // tell the user the value of RH_RF95_MAX_MESSAGE_LEN, defined in the header file 
  // RH_RF95.h (It is inside the RadioHead folder.)
  Serial.print("maximum transmit/receive packet length (bytes) is ");
  Serial.println(RH_RF95_MAX_MESSAGE_LEN);
  
  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, 
  // transmit power =+13dbM, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, 
  // CRC on, so we need to change this here. 
  
  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1);
  }
  
  Serial.print("Set Freq to: "); Serial.print(RF95_FREQ); Serial.println(" MHz");
  
  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then 
  // you can set transmitter powers from 5 to 23 dBm:
  rf95.setTxPower(23, false);

  // now ask the user if this proram is being run by a Radio Feather that
  // intiates communications, or else responds to those initiated by another processor.

  Serial.println("One Radio Feather will initiate communications with another processor.");
  Serial.println("Is this the Radio Feather that initiates communications? (Y or N)");

  while (!Serial.available()) {};

  String string_I_read = Serial.readString();

  Serial.print("Read this: "); Serial.println(string_I_read);

  if(string_I_read[0] == 'y' || string_I_read[0] == 'Y') {
    I_am_initiator = true;
    Serial.println("This processor will INITIATE communications");

  }else{
    I_am_initiator = false;
    Serial.println("This processor will RESPOND TO communications");  
  }

}

// end of setup function

///////////////////////////////////////////////////////
// loop
///////////////////////////////////////////////////////

// the loop function runs over and over again forever
void loop() {

  int return_code;
  
  if (I_am_initiator) {
    return_code = loop_initiator();
  }else{
    return_code = loop_responder();
  }
}

// end of loop

///////////////////////////////////////////////////////
// loop_initiator
///////////////////////////////////////////////////////

int loop_initiator() {

  // this function is the loop function when the Radio Feather running
  // the program is supposed to initiate communications with the other processor,
  // which will respond.

  // return code: 0 when everything is fine, -1 when not.
  
  // Wait 1 second between transmits, could also 'sleep' here!
  delay(1000); 

  // turn ON or OFF the red LED on successive entries into loop
  if (LED_toggle) {
    digitalWrite(red_LED_pin, HIGH);
  }else{
    digitalWrite(red_LED_pin, LOW);
  }

  // now flip the state of the LED toggle flag
  LED_toggle = !LED_toggle;

  // define a character array that's 20 chars long: indices go from
  // 0 to 19.
  //                      01234567890123456789
  char radiopacket[20] = "Hello World #      ";

  // "itoa" converts an integer to a null-terminated string. so here's what
  // the following line means. first increment packetnum by 1. Then convert 
  // it to a string in base 10 (the last argument) and store the string
  // starting in radiopacket[13], which is 13 elements past the first in the
  // array. 
  itoa(packetnum++, radiopacket+13, 10);

  Serial.print("\nSending the following packet: "); Serial.println(radiopacket);
  
  // put a null character in the last element of the string:
  radiopacket[19] = 0;
  
  // in the send command, note that we're "casting" the type of stuff in
  // the radiopacket array to be unsigned, 8-bit integers.
  rf95.send((uint8_t *)radiopacket, 20);

  // now wait for the trasnmission to finish
  rf95.waitPacketSent();

  Serial.println("Packet transmission finished. Waiting for reply."); 

  // Now wait for a reply. RH_RF95_MAX_MESSAGE_LEN is defined in the header file 
  // RH_RF95.h (It is inside the RadioHead folder.)
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);

  if (rf95.waitAvailableTimeout(1000))
  { 
    // Should be a reply message for us now   
    if (rf95.recv(buf, &len))
   {
      Serial.print("Got a reply: ");
      Serial.println((char*)buf);
      Serial.print("RSSI (Received Signal Strength Indicator, negative; closer to zero is better): ");
      Serial.println(rf95.lastRssi(), DEC);   

      // everything is fine, so return 0.
      return 0; 
    }
    else
    {
      Serial.println("Receive failed");
      // problems...
      return -1;      
    }

  }else{

    Serial.println("No reply, is there a listener around?");

    // alert the user...
    return -99;

  }
}

// end of loop_initiator

///////////////////////////////////////////////////////
// loop_responder
///////////////////////////////////////////////////////

int loop_responder() {

  // this function is the loop function when the Radio Feather running
  // the program is supposed to reposond to communications initiated
  // by the other processor.

  // do we have a message ready for us?
  
  if (rf95.available()) {
    
    // Yep- there should be a message packet for us now.

    // define an unsigned 8 bit array to store the packet data.
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);

    // now push the packet data into the array.
    if (rf95.recv(buf, &len)) {

      // light up the LED
      digitalWrite(red_LED_pin, HIGH);

      Serial.print("\nReceived a radio message from the other processor. Length (bytes): ");
      Serial.println(len);
      
      // RH_RF95::printBuffer("Received: ", buf, len);

      Serial.print("Message content: ");
      Serial.println((char*)buf);
      Serial.print("RSSI (Received Signal Strength Indicator, negative; closer to zero is better): ");
      Serial.println(rf95.lastRssi(), DEC);

      // Send a reply
      uint8_t data[] = "And hello back to you";
      rf95.send(data, sizeof(data));
      rf95.waitPacketSent();
      Serial.println("Sent a reply");
      digitalWrite(red_LED_pin, LOW);

      // everything is fine, so exit with a good return code.
      return 0;
    }
    else
    {
      Serial.println("Receive failed");

      // problems, so exit with an error code.
      return -1;
    }
  }

  // end of the if (rf95.available) block.

  // only get to here when there wasn't a message waiting for us.
  return -99;
}
// end of loop_responder
