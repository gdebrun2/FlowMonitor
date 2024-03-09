/*************************************************************************************
  This program is bernoulli_flowmeter.ino. Read four DPS310 pressure/temperature sensors
  using a TCA9548A I2C multiplexer, and exercise everything else on the Bernoulli 
  flowmeter

  Some of this is from Adafruit too.

  Circuit is the fall 2021 Physics 398DLP group 2 Bernoulli flow sensor.

  George Gollin
  University of Illinois
  October 2021
*************************************************************************************/

uint32_t loopcounter = 0;

#include <SPI.h>
#include <Adafruit_DPS310.h>
#include <Adafruit_Sensor.h>  
#include "Wire.h"
#include <LiquidCrystal.h>
#include "RTClib.h"
#include <Adafruit_GPS.h>
#include <string.h>
#include <SD.h>
#include <RH_RF95.h>

// TCA9548 I2C multiplexer I2C address
#define TCA9548ADDR 0x70

// need to subtract an offset from square of pressure to keep from blowing
// up the rms calculation.
#define PRESSUREOFFSET 980.0
#define PRESSURESQOFFSET 960400.0

// instantiate four DPS310 modules, each with I2C address 0x77.
// dps0
Adafruit_DPS310 dps0;
Adafruit_Sensor *dps0_temp = dps0.getTemperatureSensor();
Adafruit_Sensor *dps0_pressure = dps0.getPressureSensor();
// dps1
Adafruit_DPS310 dps1;
Adafruit_Sensor *dps1_temp = dps1.getTemperatureSensor();
Adafruit_Sensor *dps1_pressure = dps1.getPressureSensor();
// dps2
Adafruit_DPS310 dps2;
Adafruit_Sensor *dps2_temp = dps2.getTemperatureSensor();
Adafruit_Sensor *dps2_pressure = dps2.getPressureSensor();
// dps3
Adafruit_DPS310 dps3;
Adafruit_Sensor *dps3_temp = dps3.getTemperatureSensor();
Adafruit_Sensor *dps3_pressure = dps3.getPressureSensor();

// some DPS310 readings...
double temp0, pressure0;
double temp1, pressure1;
double temp2, pressure2;
double temp3, pressure3;

sensors_event_t temp0_event, pressure0_event;
sensors_event_t temp1_event, pressure1_event;
sensors_event_t temp2_event, pressure2_event;
sensors_event_t temp3_event, pressure3_event;

// we will take some averages early on.
uint32_t number_to_average = 10;
uint32_t number_so_far = 0;

double temp0avg, pressure0avg;
double temp1avg, pressure1avg;
double temp2avg, pressure2avg;
double temp3avg, pressure3avg;

double temp0sqavg, pressure0sqavg;
double temp1sqavg, pressure1sqavg;
double temp2sqavg, pressure2sqavg;
double temp3sqavg, pressure3sqavg;

double p0_minus_p1_avg_Pa;
double p0_minus_p2_avg_Pa;
double p0_minus_p3_avg_Pa;
double p1_minus_p2_avg_Pa;
double p1_minus_p3_avg_Pa;
double p2_minus_p3_avg_Pa;

double p0_minus_p1_corrected_Pa;
double p0_minus_p2_corrected_Pa;
double p0_minus_p3_corrected_Pa;
double p1_minus_p2_corrected_Pa;
double p1_minus_p3_corrected_Pa;
double p2_minus_p3_corrected_Pa;

double temp0rms, pressure0rms;
double temp1rms, pressure1rms;
double temp2rms, pressure2rms;
double temp3rms, pressure3rms;

//////////////////////////// SD stuff //////////////////////

// SD.h lives in arduino.app/COntents/Java/libraries/SD/src.
File myFile;
// chip select line to SD:
#define SD_CS 17

//////////////////////////////// LoRa radio stuff /////////////////////////////////

// "chip select" pin for the radio, used by the RadioFeather M0 to control radio
#define RFM95_CS 8
// "reset" pin for the radio. Reset when this pin is pulled low.
#define RFM95_RST 4
// An interrupt pin
#define RFM95_INT 3

// If you change to 434.0 or other frequency, must match RX's freq!
// #define RF95_FREQ 868.0
#define RF95_FREQ 915.0

// Singleton instance of the radio driver. Arguments like so:
// RH_RF95::RH_RF95(uint8_t slaveSelectPin, uint8_t interruptPin, RHGenericSPI& spi)
RH_RF95 rf95(RFM95_CS, RFM95_INT);

// packet counter
int16_t packetnum = 0;  

// here's what we'll send via LoRa
// #define RADIOPACKETLENGTH 20
// #define RADIOPACKETLENGTH 50
#define RADIOPACKETLENGTH 60
char radiopacket[RADIOPACKETLENGTH + 1];

//////////////////////////// LCD stuff //////////////////////

// initialize the LCD library by associating any needed LCD interface pins
// with the M0 pin numbers to which they are connected
const int rs = 18, en = 11, d4 = 5, d5 = 6, d6 = 9, d7 = 19;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

//////////////////////////// LED stuff //////////////////////

// There is a single external LED lit by digital pin 13 going LOW.
int LED_pin = 13;
#define LED_ON LOW
#define LED_OFF HIGH

//////////////////////////// GPS stuff //////////////////////

// I'll use the Serial1 UART (Universal Asynchronous Receiver/Transmitter)
// port to talk to the GPS through pins 0 (M0 RX) and 1
// (M0 TX).

// declare which M0 pin sees the GPS PPS signal. 
int GPS_PPS_pin = 15;

// set the name of the hardware serial port:
#define GPSSerial Serial1

// Connect to the GPS on the hardware port
Adafruit_GPS GPS(&GPSSerial);

// declare variables which we'll use to store the value (0 or 1). 
int GPS_PPS_value, GPS_PPS_value_old;

// define the synch-GPS-with-PPS command. NMEA is "National Marine Electronics 
// Association." 
#define PMTK_SET_SYNC_PPS_NMEA "$PMTK255,1*2D"

// command string to set GPS NMEA baud rate to 9,600:
#define PMTK_SET_NMEA_9600 "$PMTK251,9600*17"

// define a command to disable all NMEA outputs from the GPS except the date/time
#define PMTK_DATE_TIME_ONLY "$PMTK314,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0*29"
 
// define a command to disable ALL NMEA outputs from the GPS
#define PMTK_ALL_OFF "$PMTK314,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28"
 
// define a command to enable all NMEA outputs from the GPS
#define PMTK_ALL_ON "$PMTK314,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1*29"
 
// See https://blogs.fsfe.org/t.kandler/2013/11/17/ for additional GPS definitions.

// we don't expect a valid GPS "sentence" to be longer than this...
#define GPSMAXLENGTH 120
char GPS_sentence[GPSMAXLENGTH];
int GPS_command_string_index;

// we'll also want to convert the GPS sentence character array to a string for convenience
String GPS_sentence_string;

// pointers into parts of a GPZDA GPS data sentence whose format is
//    $GPZDA,hhmmss.sss,dd,mm,yyyy,xx,xx*CS 
//              111111111122222222223
//    0123456789012345678901234567890             
// where CS is a two-character checksum. Identify this sentence by the presence of a Z.

const int GPZDA_hour_index1 = 7;
const int GPZDA_hour_index2 = GPZDA_hour_index1 + 2;

const int GPZDA_minutes_index1 = GPZDA_hour_index2;
const int GPZDA_minutes_index2 = GPZDA_minutes_index1 + 2;
      
const int GPZDA_seconds_index1 = GPZDA_minutes_index2;
const int GPZDA_seconds_index2 = GPZDA_seconds_index1 + 2;
      
const int GPZDA_milliseconds_index1 = GPZDA_seconds_index2 + 1;   // skip the decimal point
const int GPZDA_milliseconds_index2 = GPZDA_milliseconds_index1 + 3;
      
const int GPZDA_day_index1 = GPZDA_milliseconds_index2 + 1;  // skip the comma
const int GPZDA_day_index2 = GPZDA_day_index1 + 2;
      
const int GPZDA_month_index1 = GPZDA_day_index2 + 1;
const int GPZDA_month_index2 = GPZDA_month_index1 + 2;

const int GPZDA_year_index1 = GPZDA_month_index2 + 1;
const int GPZDA_year_index2 = GPZDA_year_index1 + 4;

// here is how many non-GPZDA sentences carrying fix info to receive 
// before turning the GPZDA stuff back on.
const int maximum_fixes_to_demand = 5;

//; also define some GPGGA pointers.
const int GPGGA_command_index1 = 1;
const int GPGGA_command_index2 = GPGGA_command_index1 + 4;
const int GPGGA_commas_before_fix = 6;
const int GPGGA_crash_bumper = 50;
char GPGGA_fix;
bool we_see_satellites;
int number_of_good_fixes = 0;

// define some time variables.
unsigned long  time_ms_bumped_RTC_time_ready;

// system time from millis() at which the most recent GPS date/time
// sentence was first begun to be read
unsigned long t_GPS_read_start;

// system time from millis() at which the most recent GPS date/time
// sentence was completely parsed 
unsigned long t_GPS_read;

// system time from millis() at which the proposed bumped-by-1-second
// time is ready for downloading to the RTC
unsigned long t_bump_go; 
                
// system time from millis() at which the most recent 0 -> 1 
// transition on the GPS's PPS pin is detected
unsigned long t_GPS_PPS;  

// define some of the (self-explanatory) GPS data variables. Times/dates are UTC.
String GPS_hour_string;
String GPS_minutes_string;
String GPS_seconds_string;
String GPS_milliseconds_string;
int GPS_hour;
int GPS_minutes;
int GPS_seconds;
int GPS_milliseconds;

String GPS_day_string;
String GPS_month_string;
String GPS_year_string;
int GPS_day;
int GPS_month;
int GPS_year;

// the only kind of GPS sentence that can hold a Z, that I am allowing from the GPS,
// will carry date/time information.
bool sentence_has_a_Z;
  
// a GPGGA sentence has GPS fix data.
bool sentence_is_GPGGA;

// times for the arrival of a new data sentence and the receipt of its last character
unsigned long t_new_sentence;
unsigned long t_end_of_sentence;

// a counter
int i_am_so_bored;

//////////////////////////////////// RTC stuff //////////////////////////////

// instantiate an rtc (real time clock) object:
RTC_DS3231 rtc;

// system time from millis() at which the RTC time load is done 
unsigned long t_RTC_update;

// keep track of whether or not we have set the RTC using satellite-informed GPS data
bool good_RTC_time_from_GPS_and_satellites;

// RTC variables...
int RTC_hour;
int RTC_minutes;
int RTC_seconds;
int RTC_day;
int RTC_month;
int RTC_year;

// we will use the following to update the real time clock chip.
int RTC_hour_bumped;
int RTC_minutes_bumped;
int RTC_seconds_bumped;
int RTC_day_bumped;
int RTC_month_bumped;
int RTC_year_bumped;

// define a "DateTime" object:
DateTime now;

// limits for the timing data to be good:
const int t_RTC_update__t_GPS_PPS_min = -1;
const int t_GPS_PPS___t_bump_go_min = 200;
const int t_bump_go___t_GPS_read_min = -1;
const int t_RTC_update___t_GPS_read_min = 400;

const int t_RTC_update__t_GPS_PPS_max = 20;
const int t_GPS_PPS___t_bump_go_max = 800;
const int t_bump_go___t_GPS_read_max = 350;
const int t_RTC_update___t_GPS_read_max = 1000;

// more bookkeeping on clock setting... I will want to see several consecutive
// good reads/parses of GPS system time data to declare that all is good, and that
// we can wrap this up.
int consecutive_good_sets_so_far;
bool time_to_quit;
const int thats_enough = 5;

// it's obvious what these are:
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", 
  "Thursday", "Friday", "Saturday"};

// a "function prototype" so I can put the actual function at the end of the file:
void bump_by_1_sec(void);

////////////////////////////////////////////////////////////////////////////

void setup() 
{
  // light up the serial monitor and wait for it to be ready.
  uint32_t t1234 = millis();
  Serial.begin(115200);
  while (!Serial && millis() - t1234 < 5000) {}

  Serial.println("\n\n*************** test the DPS310s and ventilator tube insertion device ***************");

  // set the LED-illuminating pin to be a digital output.
  pinMode(LED_pin, OUTPUT);

  // make sure the LED is off.
  digitalWrite(LED_pin, LED_OFF);

  ////////// setup LCD ////////// 

  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);

  ////////// setup I2C ////////// 

  // light up the I2C comm lines
  Wire.begin(); 

  // print some info
  Serial.println("TCA9548A I2C scanner ready: let's see what is om the I2C lines.");
  Serial.println("We expect to see one DPS310 I2C address on each of ports 0, 1, 2, and 3.\n");    

  // Print a message to the LCD.
  lcd.setCursor(0, 0);
  //         0123456789012345
  lcd.print("Bernoulli Flwmtr");
  lcd.setCursor(0, 1);
  lcd.print("test            ");

  ////////// setup GPS and RTC ////////// 

  // now set up the GPS and RTC.
  int return_code = setup_GPS_RTC();

  // now set the RTC from the GPS.
  return_code = set_RTC_with_GPS();

  ////////// setup microSD memory breakout ////////// 

  return_code = setup_SD();

  ////////// setup LoRa radio ////////// 

  return_code = setup_LoRa();

  ////////// scan I2C to see what's out there ////////// 

  for (uint8_t t = 0; t < 8; t++) {
    tcaselect(t);
    Serial.print("TCA9548A Port #"); Serial.println(t);

    for (uint8_t addr = 0; addr<=127; addr++) 
    {
      if (addr == TCA9548ADDR) continue;    
      Wire.beginTransmission(addr);
      if (!Wire.endTransmission()) 
      {
        Serial.print("Found I2C 0x");  Serial.println(addr,HEX);
      }
    }
  }
  Serial.println("\n...all done scanning.\n");

  ////////// set up the DPS310 pressure/temperature sensors ////////// 

  // Try to initialise and warn if we couldn't detect the chip at port 0.
  tcaselect(0);
  if (!dps0.begin_I2C())
  {
    Serial.println("Oops ... unable to initialize the port 0 DPS310. Check your wiring!");
    while (1);
  }
  Serial.println("Found DPS310 (port 0)");
  // Setup highest precision
  dps0.configurePressure(DPS310_64HZ, DPS310_64SAMPLES);
  dps0.configureTemperature(DPS310_64HZ, DPS310_64SAMPLES);

  // now do port 1
  tcaselect(1);
  if (!dps1.begin_I2C())
  {
    Serial.println("Oops ... unable to initialize the port 1 DPS310. Check your wiring!");
    while (1);
  }
  Serial.println("Found DPS310 (port 1)");
  // Setup highest precision
  dps1.configurePressure(DPS310_64HZ, DPS310_64SAMPLES);
  dps1.configureTemperature(DPS310_64HZ, DPS310_64SAMPLES);

  // ...and port 2...
  tcaselect(2);
  if (!dps2.begin_I2C())
  {
    Serial.println("Oops ... unable to initialize the port 2 DPS310. Check your wiring!");
    while (1);
  }
  Serial.println("Found DPS310 (port 2)");
  // Setup highest precision
  dps2.configurePressure(DPS310_64HZ, DPS310_64SAMPLES);
  dps2.configureTemperature(DPS310_64HZ, DPS310_64SAMPLES);

  // ...and port 3.
  tcaselect(3);
  if (!dps3.begin_I2C())
  {
    Serial.println("Oops ... unable to initialize the port 3 DPS310. Check your wiring!");
    while (1);
  }
  Serial.println("Found DPS310 (port 3)\n");
  // Setup highest precision
  dps3.configurePressure(DPS310_64HZ, DPS310_64SAMPLES);
  dps3.configureTemperature(DPS310_64HZ, DPS310_64SAMPLES);

  Serial.println("   ");

  ////////// now read the DPS310s and do some calibrations ////////// 

  Serial.println("Now initialize DPS310s, then do a speed test (takes about 0.2 seconds per read)");

  // now read the DPS310s to get average temperatures and pressures, to use as baselines.
  // now select ports and read data.

  uint32_t tstart = millis();

  for(number_so_far = 1; number_so_far <= number_to_average; number_so_far++)
  {
    // read DPS 0
    tcaselect(0);
    // wait until there's something to read
    while (!dps0.temperatureAvailable() || !dps0.pressureAvailable()) {}
    dps0.getEvents(&temp0_event, &pressure0_event);
    temp0 = temp0_event.temperature;
    pressure0 = pressure0_event.pressure;

    // Serial.print(pressure0, 4); Serial.print(" ");
    // Serial.print(100*(pressure0 - 979.)); Serial.print(" ");
    // if(number_so_far % 20 == 0) {Serial.println(" ");}

    // read DPS 1
    tcaselect(1);
    while (!dps1.temperatureAvailable() || !dps1.pressureAvailable()) {}
    dps1.getEvents(&temp1_event, &pressure1_event);
    temp1 = temp1_event.temperature;
    pressure1 = pressure1_event.pressure;

    // read DPS 2
    tcaselect(2);
    while (!dps2.temperatureAvailable() || !dps2.pressureAvailable()) {}
    dps2.getEvents(&temp2_event, &pressure2_event);
    temp2 = temp2_event.temperature;
    pressure2 = pressure2_event.pressure;

    // read DPS 3
    tcaselect(3);
    while (!dps3.temperatureAvailable() || !dps3.pressureAvailable()) {}
    dps3.getEvents(&temp3_event, &pressure3_event);
    temp3 = temp3_event.temperature;
    pressure3 = pressure3_event.pressure;

    temp0avg = temp0avg + temp0;
    pressure0avg = pressure0avg + pressure0;
    temp1avg = temp1avg + temp1;
    pressure1avg = pressure1avg + pressure1;
    temp2avg = temp2avg + temp2;
    pressure2avg = pressure2avg + pressure2;
    temp3avg = temp3avg + temp3;
    pressure3avg = pressure3avg + pressure3;

    temp0sqavg = temp0sqavg + temp0 * temp0;
    pressure0sqavg = pressure0sqavg + pressure0 * pressure0 - PRESSURESQOFFSET;
    temp1sqavg = temp1sqavg + temp1 * temp1;
    pressure1sqavg = pressure1sqavg + pressure1 * pressure1 - PRESSURESQOFFSET;
    temp2sqavg = temp2sqavg + temp2 * temp2 * temp2avg;
    pressure2sqavg = pressure2sqavg + pressure2 * pressure2 - PRESSURESQOFFSET;
    temp3sqavg = temp3sqavg + temp3 * temp3;
    pressure3sqavg = pressure3sqavg + pressure3 * pressure3 - PRESSURESQOFFSET;
  }

  uint32_t tstop = millis();

  temp0avg = temp0avg / number_to_average;
  pressure0avg = pressure0avg / number_to_average;
  temp1avg = temp1avg / number_to_average;
  pressure1avg = pressure1avg / number_to_average;
  temp2avg = temp2avg / number_to_average;
  pressure2avg = pressure2avg / number_to_average;
  temp3avg = temp3avg / number_to_average;
  pressure3avg = pressure3avg / number_to_average;

  temp0sqavg = temp0sqavg / number_to_average;
  pressure0sqavg = pressure0sqavg / number_to_average;
  temp1sqavg = temp1sqavg / number_to_average;
  pressure1sqavg = pressure1sqavg / number_to_average;
  temp2sqavg = temp2sqavg / number_to_average;
  pressure2sqavg = pressure2sqavg / number_to_average;
  temp3sqavg = temp3sqavg / number_to_average;
  pressure3sqavg = pressure3sqavg / number_to_average;

  // now calculate offset corrections,, in Pa, not hPa...
  p0_minus_p1_avg_Pa = 100. * (pressure0avg - pressure1avg);
  p0_minus_p2_avg_Pa = 100. * (pressure0avg - pressure2avg);
  p0_minus_p3_avg_Pa = 100. * (pressure0avg - pressure3avg);
  p1_minus_p2_avg_Pa = 100. * (pressure1avg - pressure2avg);
  p1_minus_p3_avg_Pa = 100. * (pressure1avg - pressure3avg);
  p2_minus_p3_avg_Pa = 100. * (pressure2avg - pressure3avg);

  Serial.print("Time required to read all four DPS310s ");
  Serial.print(number_to_average);
  Serial.print(" times (seconds): ");
  Serial.println(float(tstop - tstart) / 1000.);

  Serial.println("Average temperatures and pressures:");

  Serial.print("DPS310 0: ");
  Serial.print(temp0avg);
  Serial.print(" *C, rms = ");
  Serial.print(sqrt(temp0sqavg - temp0avg * temp0avg));
  Serial.print("; pressure = ");
  Serial.print(pressure0avg, 6);
  Serial.print(" hPa, rms = "); 
  Serial.print(100 * sqrt(pressure0sqavg - pressure0avg * pressure0avg + PRESSURESQOFFSET), 4);
  Serial.println(" Pascals"); 

  Serial.print("DPS310 1: ");
  Serial.print(temp1avg);
  Serial.print(" *C, rms = ");
  Serial.print(sqrt(temp1sqavg - temp1avg * temp1avg));
  Serial.print("; pressure = ");
  Serial.print(pressure1avg, 6);
  Serial.print(" hPa, rms = "); 
  Serial.print(100 * sqrt(pressure1sqavg - pressure1avg * pressure1avg + PRESSURESQOFFSET), 4);
  Serial.println(" Pascals"); 

  Serial.print("DPS310 2: ");
  Serial.print(temp2avg);
  Serial.print(" *C, rms = ");
  Serial.print(sqrt(temp2sqavg - temp2avg * temp2avg));
  Serial.print("; pressure = ");
  Serial.print(pressure2avg, 6);
  Serial.print(" hPa, rms = "); 
  Serial.print(100 * sqrt(pressure2sqavg - pressure2avg * pressure2avg + PRESSURESQOFFSET), 4);
  Serial.println(" Pascals"); 

  Serial.print("DPS310 3: ");
  Serial.print(temp3avg);
  Serial.print(" *C, rms = ");
  Serial.print(sqrt(temp3sqavg - temp3avg * temp3avg));
  Serial.print("; pressure = ");
  Serial.print(pressure3avg, 6);
  Serial.print(" hPa, rms = "); 
  Serial.print(100 * sqrt(pressure3sqavg - pressure3avg * pressure3avg + PRESSURESQOFFSET), 4);
  Serial.println(" Pascals"); 

  Serial.println(" ");

  ////////// all done with setup! ////////// 

}

////////////////////////////////////////////////////////////////////////////

int setup_GPS_RTC() 
{
  // set up the GPS and RTC.

  int return_code = 0;

  // fire up the GPS.
  Serial.print("Fire up the GPS."); 

  // declare the GPS PPS pin to be an M0 input 
  pinMode(GPS_PPS_pin, INPUT);
  
  // initialize a flag and some counters
  good_RTC_time_from_GPS_and_satellites = false;
  consecutive_good_sets_so_far = 0;
  i_am_so_bored = 0;

  // 9600 NMEA is the default communication and baud rate for Adafruit MTK 3339 chipset GPS 
  // units. NMEA is "National Marine Electronics Association." 
  // Note that this serial communication path is different from the one driving the serial 
  // monitor window on your laptop.
  GPS.begin(9600);

  // initialize a flag holding the GPS PPS pin status: this pin pulses positive as soon as 
  // the seconds value rolls to the next second.
  GPS_PPS_value_old = 0;
    
  // Set the update rate to once per second. 
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ); 

  // Send a synch-with-PPS command to the GPS in hopes of having a deterministic
  // relationship between the PPS line lighting up and the GPS reporting data to us. According
  // to the manufacturer, the GPS will start sending us a date/time data sentence about 170
  // milliseconds after the PPS line transitions fom 0 to 1. 
  GPS.sendCommand(PMTK_SET_SYNC_PPS_NMEA);

  // turn on RMC (recommended minimum) and GGA (fix data) including altitude
  // GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);

  // turn on all data
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_ALLDATA);
  
  // turn off most GPS outputs to reduce the rate of stuff coming at us.
  // GPS.sendCommand(PMTK_DATE_TIME_ONLY);

  // this keeps track of where in the string of characters of a GPS data sentence we are.
  GPS_command_string_index = 0;

  // more initialization
  sentence_has_a_Z = false;
  sentence_is_GPGGA = false;
  we_see_satellites = false;

  time_to_quit = false;

  // fire up the RTC.
  Serial.print("Fire up the RTC. return code is "); 
  return_code = rtc.begin();
  Serial.println(rtc.begin());

  // problems?
  if(!return_code) {
    Serial.println("RTC wouldn't respond so bail out.");
    while (1) {};
  }

  // now try read back the RTC to check.       
  delay(500);
  
  now = rtc.now();
  Serial.print("Now read back the RTC to check during setup. ");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  if(now.minute() < 10)   Serial.print(0);
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  if(now.second() < 10)   Serial.print(0);
  Serial.print(now.second(), DEC);

  Serial.print("   Date (dd/mm/yyyy): ");
  if(int(now.day()) < 10) Serial.print("0");
  Serial.print(now.day(), DEC); Serial.print('/');
  if(int(now.month()) < 10) Serial.print("0");
  Serial.print(now.month(), DEC); Serial.print("/");
  Serial.println(now.year(), DEC);

  // Print a message to the LCD.
  lcd.setCursor(0, 0);
  lcd.print("Now looking for ");
  lcd.setCursor(0, 1);
  lcd.print("GPS satellites  ");

  // print information about the types of GPS sentences.
  Serial.println("Kinds of GPS sentences");
  Serial.println("   $GPBOD - Bearing, origin to destination");
  Serial.println("   $GPBWC - Bearing and distance to waypoint, great circle");
  Serial.println("   $GPGGA - Global Positioning System Fix Data");
  Serial.println("   $GPGLL - Geographic position, latitude / longitude");
  Serial.println("   $GPGSA - GPS DOP and active satellites ");
  Serial.println("   $GPGSV - GPS Satellites in view");
  Serial.println("   $GPHDT - Heading, True");
  Serial.println("   $GPR00 - List of waypoints in currently active route");
  Serial.println("   $GPRMA - Recommended minimum specific Loran-C data");
  Serial.println("   $GPRMB - Recommended minimum navigation info");
  Serial.println("   $GPRMC - Recommended minimum specific GPS/Transit data");
  Serial.println("   $GPRTE - Routes");
  Serial.println("   $GPTRF - Transit Fix Data");
  Serial.println("   $GPSTN - Multiple Data ID");
  Serial.println("   $GPVBW - Dual Ground / Water Speed");
  Serial.println("   $GPVTG - Track made good and ground speed");
  Serial.println("   $GPWPL - Waypoint location");
  Serial.println("   $GPXTE - Cross-track error, Measured");
  Serial.println("   $GPZDA - Date & Time\n");

  // all done!
  return return_code;

}

////////////////////////////////////////////////////////////////////////////

int set_RTC_with_GPS()
{
  // set the RTC with the GPS.

  int return_code = 0;

  // ******************************************************************************
  /*

  First things first: check to see if we are we done setting the RTC. In order to 
  declare victory and exit, we'll need the following to happen. 

  Definitions:

    t_GPS_read    system time from millis() at which the most recent GPS date/time
                  sentence was completely parsed BEFORE the most recent PPS 0 -> 1 
                  transition was detected 
                      
    t_bump_go     system time from millis() at which the proposed bumped-by-1-second
                  time is ready for downloading to the RTC
    
    t_GPS_PPS     system time from millis() at which the most recent 0 -> 1 
                  transition on the GPS's PPS pin is detected

    t_RTC_update  system time from millis() at which the RTC time load is done 

  Typical timing for an event:   

    t_GPS_read    17,961    
    t_bump_go     17,971 (t_GPS_read +  10 ms)    
    t_GPS_PPS     18,597 (t_bump_go  + 626 ms)    
    t_RTC_update  18,598 (t_GPS_PPS  +   1 ms)

  Every once in a while we might miss the PPS 0 -> 1 transition, or the GPS might 
  not feed us a data sentence. So let's impose the following criteria.

  0 ms   <= t_RTC_update - t_GPS_PPS  <= 10 ms
  200 ms <= t_GPS_PPS - t_bump_go     <= 800 ms
  0 ms   <= t_bump_go - t_GPS_read    <= 50 ms
  400 ms <= t_RTC_update - t_GPS_read <= 1000 ms

  */

  // we need to loop here, so we can read GPS data, one character per pass through this loop.

  while(1)
  {
    if(time_to_quit) 
    {
      // print a message to the serial monitor, but only once.
      if (i_am_so_bored == 0) Serial.print("\n\nTime to quit! We have set the RTC.");

      // Print a message to the LCD each pass through, updating the time.
      lcd.setCursor(0, 0);
      //         0123456789012345
      lcd.print("RTC is now set  ");

      // blank the LCD's second line 
      lcd.setCursor(0, 1);
      lcd.print("                ");

      // print the time
      lcd.setCursor(0, 1);
      now = rtc.now();
      
      if(now.hour() < 10)   lcd.print(0);
      lcd.print(now.hour(), DEC);
      
      lcd.print(':');
      if(now.minute() < 10)   lcd.print(0);
      lcd.print(now.minute());
      
      lcd.print(':');
      if(now.second() < 10)   lcd.print(0);
      lcd.print(now.second());

      delay(50);

      // increment a counter
      i_am_so_bored++;

      return_code = 0;
      return return_code;
    }

    // *******************************************************************************

    // now check to see if we just got a PPS 0 -> 1 transition, indicating that the
    // GPS clock has just ticked over to the next second.

    GPS_PPS_value = digitalRead(GPS_PPS_pin);
    
    // did we just get a 0 -> 1 transition?
    if (GPS_PPS_value == 1 && GPS_PPS_value_old == 0) 
    {  
      Serial.print("\nJust saw a PPS 0 -> 1 transition at time (ms) = ");
      t_GPS_PPS = millis();
      Serial.println(t_GPS_PPS);

      // load the previously established time values into the RTC now.
      if (good_RTC_time_from_GPS_and_satellites) {

        // now set the real time clock to the bumped-by-one-second value that we have 
        // already calculated. To set the RTC with an explicit date & time, for example 
        // January 21, 2014 at 3am you would call
        // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
      
        rtc.adjust(DateTime(int(RTC_year_bumped), int(RTC_month_bumped), int(RTC_day_bumped), int(RTC_hour_bumped), 
          int(RTC_minutes_bumped), int(RTC_seconds_bumped)));

        // take note of when we're back from setting the real time clock:
        t_RTC_update = millis();

        // Serial.print("Just returned from updating RTC at system t = "); Serial.println(t_RTC_update);

        Serial.print("Proposed new time fed to the RTC was ");
        Serial.print(RTC_hour_bumped, DEC); Serial.print(':');
        if(RTC_minutes_bumped < 10) Serial.print("0");
        Serial.print(RTC_minutes_bumped, DEC); Serial.print(':');
        if(RTC_seconds_bumped < 10) Serial.print("0");
        Serial.print(RTC_seconds_bumped, DEC); 
        Serial.print("   Date (dd/mm/yyyy): ");
        if(int(RTC_day_bumped) < 10) Serial.print("0");
        Serial.print(RTC_day_bumped, DEC); Serial.print('/');
        if(RTC_month_bumped < 10) Serial.print("0");
        Serial.print(RTC_month_bumped, DEC); Serial.print("/");
        Serial.println(RTC_year_bumped, DEC);  

        // now read back the RTC to check.       
        now = rtc.now();
        Serial.print("Now read back the RTC to check. ");
        Serial.print(now.hour(), DEC);
        Serial.print(':');
        if(now.minute() < 10)   Serial.print(0);
        Serial.print(now.minute(), DEC);
        Serial.print(':');
        if(now.second() < 10)   Serial.print(0);
        Serial.print(now.second(), DEC);

        Serial.print("   Date (dd/mm/yyyy): ");
        if(int(now.day()) < 10) Serial.print("0");      
        Serial.print(now.day(), DEC); Serial.print('/');
        if(int(now.month()) < 10) Serial.print("0");
        Serial.print(now.month(), DEC); Serial.print("/");
        Serial.println(now.year(), DEC);
        
        // now that we've used this GPS value, set the following flag to false:
        good_RTC_time_from_GPS_and_satellites = false;
        consecutive_good_sets_so_far++;
        time_to_quit = consecutive_good_sets_so_far >= thats_enough;
      }
    }

    GPS_PPS_value_old = GPS_PPS_value;

    // *******************************************************************************
    // read data from the GPS; do this one character per pass through function loop.
    // when synched to the PPS pin, the GPS sentence will start arriving about 170 ms
    // after the PPS line goes high, according to the manufacturer of the MTK3339 GPS
    // chipset. So we need to start by seeing if there's been a PPS 0 -> 1 transition.
    // *******************************************************************************

    char c;

    // is there anything new to be read?

    if(GPSSerial.available()) {

      // read the character
      c = GPS.read();

      // a "$" indicates the start of a new sentence.
      if (c == '$') 
      {
        //reset the array index indicating where we put the characters as we build the GPS sentence.
        GPS_command_string_index = 0;
        t_new_sentence = millis();
        sentence_has_a_Z = false;
        sentence_is_GPGGA = false;

      } else {
      GPS_command_string_index++;
    }

      // build up the data sentence, one character at a time.
      GPS_sentence[GPS_command_string_index] = c;

      // are we reading a sentence from the GPS that carries date/time information? The
      // format is this: 
      //    $GPZDA,hhmmss.sss,dd,mm,yyyy,xx,xx*CS 
      // where CS is a checksum. Identify this kind of sentence by the presence of a Z.

      if (c == 'Z') 
      {
        sentence_has_a_Z = true;
      }
      
      // a "*" indicates the end of the sentence, except for the two-digit checksum and the CR/LF.
      if (c == '*') 
      {
        t_end_of_sentence = millis();
        t_GPS_read = t_end_of_sentence;
        // Serial.print("Beginning, end of reception of latest GPS sentence: "); Serial.print(t_new_sentence);
        // Serial.print(", "); Serial.println(t_end_of_sentence);

        // convert GPS data sentence from a character array to a string.
        GPS_sentence_string = String(GPS_sentence);

        // print the GPS sentence
        Serial.print("New GPS_sentence_string is "); 
        Serial.println(GPS_sentence_string.substring(0, GPS_command_string_index+1));

        // now parse the string if it corresponds to a date/time message.
        if (sentence_has_a_Z) 
        {
          GPS_hour_string = GPS_sentence_string.substring(GPZDA_hour_index1, GPZDA_hour_index2);
          GPS_minutes_string = GPS_sentence_string.substring(GPZDA_minutes_index1, GPZDA_minutes_index2);
          GPS_seconds_string = GPS_sentence_string.substring(GPZDA_seconds_index1, GPZDA_seconds_index2);
          GPS_milliseconds_string = GPS_sentence_string.substring(GPZDA_milliseconds_index1, GPZDA_milliseconds_index2);
          GPS_day_string = GPS_sentence_string.substring(GPZDA_day_index1, GPZDA_day_index2);
          GPS_month_string = GPS_sentence_string.substring(GPZDA_month_index1, GPZDA_month_index2);
          GPS_year_string = GPS_sentence_string.substring(GPZDA_year_index1, GPZDA_year_index2);
    
          Serial.print("GPS time (UTC) in this sentence is " + GPS_hour_string + ":" + GPS_minutes_string + ":" + 
          GPS_seconds_string + "." + GPS_milliseconds_string);
          Serial.println("      dd/mm/yyyy = " + GPS_day_string + "/" + GPS_month_string + "/" + GPS_year_string);
    
          // now convert to integers
          GPS_hour = GPS_hour_string.toInt();
          GPS_minutes = GPS_minutes_string.toInt();
          GPS_seconds = GPS_seconds_string.toInt();
          GPS_milliseconds = GPS_milliseconds_string.toInt();
          GPS_day = GPS_day_string.toInt();
          GPS_month = GPS_month_string.toInt();
          GPS_year = GPS_year_string.toInt();
    
          // now set the RTC variables.
          RTC_hour = GPS_hour;
          RTC_minutes = GPS_minutes;
          RTC_seconds = GPS_seconds;
          RTC_day = GPS_day;
          RTC_month = GPS_month;
          RTC_year = GPS_year;
    
          // now try bumping everything by 1 second.
          bump_by_1_sec();
    
          t_bump_go = millis();
    
          // set a flag saying that we have a good proposed time to load into the RTC. We
          // will load this the next time we see a PPS 0 -> 1 transition.
          good_RTC_time_from_GPS_and_satellites = true;
          
        }

        // now parse the string if it corresponds to a GPGGA "fix data" message.
        // here is a typical GPGGA sentence:
        //    $GPGGA,165838.000,4006.9608,N,08815.4365,W,1,07,0.99,252.1,M,-33.9,M,,*
        // note the fix quality immediately after the comma that follows the "W" or "E" in
        // the longitude data. For the fix data: 1 or 2 are both good.

        // skip a GPZDA sentence, if that's what we have.
        if (!sentence_has_a_Z) 
        {
          // now check to see if characters 1 - 5 are "GPGGA"
          sentence_is_GPGGA = true;
          char testit[] = "$GPGGA";

          // read one character at a time.
          for(int ijk = GPGGA_command_index1; ijk <= GPGGA_command_index2; ijk++)
          {
            if(GPS_sentence_string[ijk] != testit[ijk]) {sentence_is_GPGGA = false;}
          }

          // if sentence_is_GPGGA is still true, we are in luck.

          if(sentence_is_GPGGA)
          {
            // look for the "W" oir "E" now. set GPGGA_fix to 0 in case we don't find anything good.
            GPGGA_fix = '0';

            for(int ijk = 0; ijk <= GPGGA_crash_bumper; ijk++)
            {
              if(GPS_sentence_string[ijk] == 'W' || GPS_sentence_string[ijk] == 'E')
              {
                // now wwe know where the fix datum resides, so break out.
                GPGGA_fix = GPS_sentence_string[ijk + 2];
                break;
              }
            }

            // is there a 1 or 2 there? Note thayt the are characters, not integers.
            we_see_satellites = (GPGGA_fix == '1' || GPGGA_fix == '2');

            Serial.print("  Fix parameter is ");
            Serial.print(GPGGA_fix);

            Serial.print(". we_see_satellites is ");
            Serial.print(we_see_satellites);

            if(we_see_satellites) 
            {
              number_of_good_fixes++;
              Serial.print(". number_of_good_fixes is now ");
              Serial.print(number_of_good_fixes);

              // Print a message to the LCD.
              lcd.setCursor(0, 0);
              lcd.print("Now have a GPS  ");
              lcd.setCursor(0, 1);
              lcd.print("satellite fix!  ");

            }

            Serial.println(" ");

            // is it time to turn back on the GPZDA sentences?
            if(number_of_good_fixes >= maximum_fixes_to_demand)
            {
              GPS.sendCommand(PMTK_DATE_TIME_ONLY);
              Serial.println("\nNow turning on the GPZDA date/time records from GPS\n");
            }

          } 

        }
      }
    }  
  }
}
////////////////////////////////////////////////////////////////////////////

void loop() 
{
  // now select ports and read data.

  uint32_t tstart = millis();
  bool print_stuff = true;

  // turn on the LED.
  digitalWrite(LED_pin, LED_ON);

  // read DPS 0
  tcaselect(0);
  // wait until there's something to read
  while (!dps0.temperatureAvailable() || !dps0.pressureAvailable()) {}
  dps0.getEvents(&temp0_event, &pressure0_event);
  temp0 = temp0_event.temperature;
  pressure0 = pressure0_event.pressure;

  // read DPS 1
  tcaselect(1);
  while (!dps1.temperatureAvailable() || !dps1.pressureAvailable()) {}
  dps1.getEvents(&temp1_event, &pressure1_event);
  temp1 = temp1_event.temperature;
  pressure1 = pressure1_event.pressure;

  // read DPS 2
  tcaselect(2);
  while (!dps2.temperatureAvailable() || !dps2.pressureAvailable()) {}
  dps2.getEvents(&temp2_event, &pressure2_event);
  temp2 = temp2_event.temperature;
  pressure2 = pressure2_event.pressure;

  // read DPS 3
  tcaselect(3);
  while (!dps3.temperatureAvailable() || !dps3.pressureAvailable()) {}
  dps3.getEvents(&temp3_event, &pressure3_event);
  temp3 = temp3_event.temperature;
  pressure3 = pressure3_event.pressure;

  uint32_t tstop = millis();

  number_so_far++;

  // calculate pressure differences, corrected for intrinsic offsets of
  // the different DPS310s:
  p0_minus_p1_corrected_Pa = 100. * (pressure0 - pressure1) - p0_minus_p1_avg_Pa;
  p0_minus_p2_corrected_Pa = 100. * (pressure0 - pressure2) - p0_minus_p2_avg_Pa;
  p0_minus_p3_corrected_Pa = 100. * (pressure0 - pressure3) - p0_minus_p3_avg_Pa;
  p1_minus_p2_corrected_Pa = 100. * (pressure1 - pressure2) - p1_minus_p2_avg_Pa;
  p1_minus_p3_corrected_Pa = 100. * (pressure1 - pressure3) - p1_minus_p3_avg_Pa;
  p2_minus_p3_corrected_Pa = 100. * (pressure2 - pressure3) - p2_minus_p3_avg_Pa;

  if(print_stuff)
  {
    Serial.println(F("----====<<<< >>>>====----"));
    print_RTC();
    Serial.print(F("Temperature 0 = "));
    Serial.print(temp0);
    Serial.print(" *C; change from average = ");
    Serial.println(temp0 - temp0avg);
    Serial.print(F("Pressure 0 = "));
    Serial.print(pressure0, 6);
    Serial.print(" hPa; change from average = "); 
    Serial.print(100. * (pressure0 - pressure0avg), 4);
    Serial.println(" Pascals"); 

    Serial.print(F("Temperature 1 = "));
    Serial.print(temp1);
    Serial.print(" *C; change from average = ");
    Serial.println(temp1 - temp1avg);
    Serial.print(F("Pressure 1 = "));
    Serial.print(pressure1, 6);
    Serial.print(" hPa; change from average = "); 
    Serial.print(100. * (pressure1 - pressure1avg), 4);
    Serial.println(" Pascals"); 

    Serial.print(F("Temperature 2 = "));
    Serial.print(temp2);
    Serial.print(" *C; change from average = ");
    Serial.println(temp2 - temp2avg);
    Serial.print(F("Pressure 2 = "));
    Serial.print(pressure2, 6);
    Serial.print(" hPa; change from average = "); 
    Serial.print(100. * (pressure2 - pressure2avg), 4);
    Serial.println(" Pascals"); 

    Serial.print(F("Temperature 3 = "));
    Serial.print(temp3);
    Serial.print(" *C; change from average = ");
    Serial.println(temp3 - temp3avg);
    Serial.print(F("Pressure 3 = "));
    Serial.print(pressure3, 6);
    Serial.print(" hPa; change from average = "); 
    Serial.print(100. * (pressure3 - pressure3avg), 4);
    Serial.println(" Pascals"); 

    Serial.println();
    Serial.print("Time (ms) to read all four DPS310s: ");
    Serial.println(tstop - tstart);
    Serial.print("\n");

    Serial.println("Uncorrected pressure differences");
    Serial.print("P0 - P1 (Pa): ");
    Serial.println(100. * (pressure0 - pressure1));
    Serial.print("P0 - P2 (Pa): ");
    Serial.println(100. * (pressure0 - pressure2));
    Serial.print("P0 - P3 (Pa): ");
    Serial.println(100. * (pressure0 - pressure3));

    Serial.print("P1 - P2 (Pa): ");
    Serial.println(100. * (pressure1 - pressure2));
    Serial.print("P1 - P3 (Pa): ");
    Serial.println(100. * (pressure1 - pressure3));

    Serial.print("P2 - P3 (Pa): ");
    Serial.println(100. * (pressure2 - pressure3));

    Serial.println(" ");

    Serial.println("CORRECTED for average 0, 1, 2, 3 offsets");
    Serial.print("P0 - P1 (Pa): ");
    Serial.println(p0_minus_p1_corrected_Pa);
    Serial.print("P0 - P2 (Pa): ");
    Serial.println(p0_minus_p2_corrected_Pa);
    Serial.print("P0 - P3 (Pa): ");
    Serial.println(p0_minus_p3_corrected_Pa);
    Serial.print("P1 - P2 (Pa): ");
    Serial.println(p1_minus_p2_corrected_Pa);
    Serial.print("P1 - P3 (Pa): ");
    Serial.println(p1_minus_p3_corrected_Pa);
    Serial.print("P2 - P3 (Pa): ");
    Serial.println(p2_minus_p3_corrected_Pa);
    
    // Print a message to the LCD.
    lcd.setCursor(0, 0);
    //         0123456789012345
    lcd.print("dp 0,1,2        ");
    lcd.setCursor(9, 0);
    lcd.print(100. * (pressure0 - pressure1) - p0_minus_p1_avg_Pa, 2);
    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print(100. * (pressure0 - pressure2) - p0_minus_p2_avg_Pa, 2);
    lcd.print(" ");
    lcd.print(100. * (pressure1 - pressure2) - p1_minus_p2_avg_Pa, 2);

    delay(1000);

    // turn the LED off.
    digitalWrite(LED_pin, LED_OFF);

    // send a radio message now, loading the radiopacket array before sending the
    // message. Note that the string must be 50 characters long, or less.

    char p0p1_corr[10];
    char p0p2_corr[10];
    char p0p3_corr[10];
    char p1p2_corr[10];
    char p1p3_corr[10];
    char p2p3_corr[10];

    // now convert doubles to strings.
    sprintf(p0p1_corr, "%.2f", p0_minus_p1_corrected_Pa);
    sprintf(p0p2_corr, "%.2f", p0_minus_p2_corrected_Pa);
    sprintf(p0p3_corr, "%.2f", p0_minus_p3_corrected_Pa);
    sprintf(p1p2_corr, "%.2f", p1_minus_p2_corrected_Pa);
    sprintf(p1p3_corr, "%.2f", p1_minus_p3_corrected_Pa);
    sprintf(p2p3_corr, "%.2f", p2_minus_p3_corrected_Pa);

    //                             1111111111222222222233333333334444444444
    //                   01234567890123456789012345678901234567890123456789
    strcpy(radiopacket, "                                                  ");

    // now copy some of this into the string to be sent.
    int ioff = 0;
    int the_len;

    the_len = strlen(p0p1_corr);
    for(int ichar = 0; ichar < the_len; ichar++) {radiopacket[ichar + ioff] = p0p1_corr[ichar];}
    ioff = ioff + the_len + 1;

    the_len = strlen(p0p2_corr);
    for(int ichar = 0; ichar < the_len; ichar++) {radiopacket[ichar + ioff] = p0p2_corr[ichar];}
    ioff = ioff + the_len + 1;

    the_len = strlen(p0p3_corr);
    for(int ichar = 0; ichar < the_len; ichar++) {radiopacket[ichar + ioff] = p0p3_corr[ichar];}
    ioff = ioff + the_len + 1;

    the_len = strlen(p1p2_corr);
    for(int ichar = 0; ichar < the_len; ichar++) {radiopacket[ichar + ioff] = p1p2_corr[ichar];}
    ioff = ioff + the_len + 1;

    the_len = strlen(p1p3_corr);
    for(int ichar = 0; ichar < the_len; ichar++) {radiopacket[ichar + ioff] = p1p3_corr[ichar];}
    ioff = ioff + the_len + 1;

    the_len = strlen(p2p3_corr);
    for(int ichar = 0; ichar < the_len; ichar++) {radiopacket[ichar + ioff] = p2p3_corr[ichar];}

    send_LoRa();

    delay(1000);

    Serial.println(" ");

  }

}

////////////////////////////////////////////////////////////////////////////

void tcaselect(uint8_t i) {

  // DPS310 I2C address is 0x77
  // DS3231 I2C address is 0x68

  if (i > 7) return;
  Wire.beginTransmission(TCA9548ADDR);
  Wire.write(1 << i);
  Wire.endTransmission();  

}

////////////////////////////////////////////////////////////////////////////

void bump_by_1_sec(){

  // bump the RTC clock time by 1 second relative to the GPS value reported 
  // a few hundred milliseconds ago. I am using global variables for the ease
  // of doing this. Note that we're going to need to handle roll-overs from 59 
  // seconds to 0, and so forth.

    bool bump_flag;
    int place_holder;

    bool debug_echo = false;

    RTC_seconds_bumped = RTC_seconds + 1;

    // use "place_holder" this way so the timings through the two branches of the if blocks 
    // are the same
    place_holder = RTC_seconds + 1;
    
    if(int(RTC_seconds_bumped) >= 60) {
      bump_flag = true;
      RTC_seconds_bumped = 0;
      }else{
      bump_flag = false;
      RTC_seconds_bumped = place_holder;
      }
      
    place_holder = RTC_minutes + 1;
    
    // do we also need to bump the minutes?  
    if (bump_flag) {
      RTC_minutes_bumped = place_holder;
      }else{
      RTC_minutes_bumped = RTC_minutes;
      }

    // again, do this to equalize the time through the two branches of the if block
    place_holder = RTC_minutes_bumped;
    
    if(int(RTC_minutes_bumped) >= 60) {
      bump_flag = true;
      RTC_minutes_bumped = 0;
      }else{
      bump_flag = false;
      RTC_minutes_bumped = place_holder;
      }

    place_holder = RTC_hour + 1;
    
    // do we also need to bump the hours?  
    if (bump_flag) {
      RTC_hour_bumped = place_holder;
      }else{
      RTC_hour_bumped = RTC_hour;
      }

    place_holder = RTC_hour;

    if(int(RTC_hour_bumped) >= 24) {
      bump_flag = true;
      RTC_hour_bumped = 0;
      }else{
      bump_flag = false;
      RTC_hour_bumped = place_holder;
      }

    place_holder = RTC_day + 1;
    
    // do we also need to bump the days?  
    if (bump_flag) {
      RTC_day_bumped = place_holder;
      }else{
      RTC_day_bumped = RTC_day;
      }

    // do we need to bump the month too? Note the stuff I do to make both paths
    // through the if blocks take the same amount of execution time.
    
    int nobody_home;
    int days_in_month = 31;

    // 30 days hath September, April, June, and November...
    if (int(RTC_month) == 9 || int(RTC_month) == 4 || int(RTC_month) == 6 || int(RTC_month) == 11) {
      days_in_month = 30;
    }else{
      nobody_home = 99;
    }
      
    // ...all the rest have 31, except February...
    if (int(RTC_month) == 2 && (int(RTC_year) % 4)) {
      days_in_month = 28;
    }else{
      nobody_home = 99;
    }
    
    // ...leap year!
    if (int(RTC_month) == 2 && !(int(RTC_year) % 4)) {
      days_in_month = 29;
    }else{
      nobody_home = 99;
    }

    place_holder = RTC_day_bumped;
    
    if(int(RTC_day_bumped) > days_in_month) {
      bump_flag = true;
      RTC_day_bumped = 1;
      }else{
      bump_flag = false;
      RTC_day_bumped = place_holder;
      }

    if (bump_flag) {
      RTC_month_bumped = RTC_month + 1;
      }else{
      RTC_month_bumped = RTC_month;
      }

    place_holder = RTC_month_bumped;
              
    //... and also bump the year?
    
    if(int(RTC_month_bumped) > 12) {
      bump_flag = true;
      RTC_month_bumped = 1;
      }else{
      bump_flag = false;
      RTC_month_bumped = place_holder;
      }

    if (bump_flag) {
      RTC_year_bumped = RTC_year + 1;
      }else{
      RTC_year_bumped = RTC_year;
      }

    // keep track of when we have the proposed RTC time value ready for loading
    time_ms_bumped_RTC_time_ready = millis();

    if (debug_echo) {
      // now print the newly bumped time:
      Serial.print("Now have a proposed (1 second bumped) time ready at (ms) ");
      Serial.println(time_ms_bumped_RTC_time_ready, DEC);       
      Serial.print("Proposed (1 second bumped) time: ");
      Serial.print(RTC_hour_bumped, DEC); Serial.print(':');
      if(RTC_minutes_bumped < 10) Serial.print("0");
      Serial.print(RTC_minutes_bumped, DEC); Serial.print(':');
      if(RTC_seconds_bumped < 10) Serial.print("0");
      Serial.print(RTC_seconds_bumped, DEC); 
      Serial.print("   Date (dd/mm/yyyy): ");
      if(RTC_day_bumped < 10) Serial.print("0");      
      Serial.print(RTC_day_bumped, DEC); Serial.print('/');
      if(RTC_month_bumped < 10) Serial.print("0");
      Serial.print(RTC_month_bumped, DEC); Serial.print("/");
      Serial.println(RTC_year_bumped, DEC);
    }
  
}    

/////////////////////////////////////////////////////////////////////////

int setup_SD()
{
  int return_code = 0;

  // because the radio also uses SPI I need to pull
  // the radio's chip select line HIGH to disable the radio for the moment.
  Serial.println("setuo_SD: first disable the LoRa radio");

  // RFM95_RST is used by the processor as a reset pin for the radio.
  // I think it's actually a reset-bar line: reset when pulled low.
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  // make very sure the reset line has time to settle.
  delay(100);

  // manual reset, with 100 millisecond dwell times in each state
  digitalWrite(RFM95_RST, LOW);
  delay(100);

  digitalWrite(RFM95_RST, HIGH);
  delay(100);

  pinMode(RFM95_CS, OUTPUT);
  // disable the radio for the moment by jammming the chip select line high.
  digitalWrite(RFM95_CS, HIGH);

  Serial.println("Initializing SD card...");

  if (!SD.begin(SD_CS)) {
    Serial.println("SD initialization failed!");
    return_code = -1;
    return return_code;
  }

  Serial.println("SD initialization done. Now open test.txt");

  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  myFile = SD.open("test.txt", FILE_WRITE);

  uint32_t millis_start = millis();

  // if the file opened okay, write to it:
  if (myFile) 
  {
    Serial.print("Writing to test.txt...");

    myFile.println("************ testing 1, 2, 3 ************");
    for(int ijk = 0; ijk < 5; ijk++)
    {
      myFile.print("   current millis() and micros() values are  ");
      myFile.print(millis());
      myFile.print("  ");
      myFile.println(micros());
    }
    myFile.println("************ all done! ************");

    // close the file:
    myFile.close();
    Serial.println("done: just closed file.\n");

  } else {

    // if the file didn't open, print an error:
    Serial.println("error opening test.txt\n");
    return_code = -2;
    return return_code;
  }

  // re-open the file for reading
  Serial.println("Now open test.txt, read it, and echo it to the serial monitor.");
  Serial.print("Initial millis()n value was ");
  Serial.println(millis_start);

  myFile = SD.open("test.txt");

  if (myFile) 
  {
    Serial.println("test.txt contents:");

    // read from the file until there's nothing else in it:
    while (myFile.available()) 
    {
      Serial.write(myFile.read());
    }

    // close the file:
    myFile.close();
    Serial.println("we are finished: just closed file.\n");
  } else {

    // if the file didn't open, print an error:
    Serial.println("error opening test.txt");
    return_code = -3;
    return return_code;
  }

  return return_code;
}

/////////////////////////////////////////////////////////////////////////

int setup_LoRa()
{
  // set up the radio.

  int return_code = 0;

  // set the control pins' modes
  pinMode(RFM95_RST, OUTPUT);
  pinMode(RFM95_CS, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  // let's make sure the SD isn't getting in our way.
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);

  // manual reset: pull LOW to reset, put HIGH to allow LoRa to  function. 
  // Since the group 1 PCB doesn't connect the RST imput, take this into account.

  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  while (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    return_code = -1;
    return return_code;
  }

  Serial.println("LoRa radio init OK!");

  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  // but we want to use 915 MHz...
  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    return_code = -2;
    return return_code;
  }
  Serial.print("Just set Freq to: "); Serial.println(RF95_FREQ);
  
  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then 
  // you can set transmitter powers from 5 to 23 dBm:
  rf95.setTxPower(23, false);
  Serial.println("Just set LoRa power.");

  // now load the radiopacket array before sending a LoRa message. Note that the 
  // string must be 50 characters long, or less.
  //                             1111111111222222222233333333334444444444
  //                   01234567890123456789012345678901234567890123456789
  strcpy(radiopacket, "Bernoulli flowmeter, George Gollin, UIUC");
  return_code = send_LoRa();

  return return_code;
}

/////////////////////////////////////////////////////////////////////////

int send_LoRa()
{
  // send a LoRa radio message of up to 50 characters.

  // contents of the character array radiopacket (a global) will be transmitted,
  // which should be loaded before the call to send_LoRa.

  int return_code = 0;

  // let's make sure the SD isn't getting in our way.
  digitalWrite(SD_CS, HIGH);

  Serial.println("\nRadio Feather M0 sending to rf95 listener");
  //                             1111111111222222222233333333334444444444
  //                   01234567890123456789012345678901234567890123456789
  // strcpy(radiopacket, "Bernoulli flowmeter, George Gollin, UIUC          ");
  // strcpy(radiopacket, "Bernoulli flowmeter, George Gollin, UIUC");

  // Send a message to rf95_server
  int ijk = RADIOPACKETLENGTH - 7;
  int lmn = strlen(radiopacket);
  if(lmn < ijk)
  {
    radiopacket[lmn] = ' ';
    itoa(packetnum++, radiopacket + lmn, 10);
  }

  // itoa(packetnum++, radiopacket + ijk, 10);
  Serial.print("Sending the radio string "); Serial.print(radiopacket);
  Serial.println("'");
  radiopacket[RADIOPACKETLENGTH - 1] = 0;

  lcd.setCursor(0, 0);
  lcd.print("Sending packet  ");
  lcd.setCursor(0, 1);
  lcd.print("to base station ");
  
  delay(10);
  rf95.send((uint8_t *)radiopacket, RADIOPACKETLENGTH);
  
  Serial.println("Waiting for packet to complete..."); delay(10);
  rf95.waitPacketSent();

  // Now wait for a reply
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);
  
  Serial.println("Packet sent. Wait for reply..."); 

  lcd.setCursor(0, 1);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  //         0123456789012345
  lcd.print("Packet sent     ");

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
      digitalWrite(LED_pin, HIGH);
      RH_RF95::printBuffer("Bernoulli flowmeter has received reply: ", buf, len);
      Serial.print("That message, as characters, is '");
      Serial.print((char*)buf);
      Serial.println("'");
      Serial.print("RSSI (signal strength, in dB): ");
      Serial.println(rf95.lastRssi(), DEC);
      Serial.print("Time (ms) spent waiting for reply: ");
      Serial.println(millis() - millis_now);
      Serial.println("Radio transmission and acknowledgement are OK.\n");

      lcd.setCursor(0, 0);
      lcd.print("We have received");
      lcd.setCursor(0, 1);
      //         0123456789012345
      lcd.print("a reply.        ");
      lcd.setCursor(9, 1);
      lcd.print(packetnum);
    }
    else
    {
      Serial.println("Receive (as a reply-to) failed");
    }
  } else {
    Serial.println("Timed out waiting for reply.");
  }
  return return_code;
}

/////////////////////////////////////////////////////////////////////////

void print_RTC()
{
  // print the time, according to the DS3231 real time clock.

  now = rtc.now();
  Serial.print("Current time: ");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  if(now.minute() < 10)   Serial.print(0);
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  if(now.second() < 10)   Serial.print(0);
  Serial.print(now.second(), DEC);

  Serial.print(" (UTC: CDT + 5, CST + 6).  Date (dd/mm/yyyy): ");
  if(int(now.day()) < 10) Serial.print("0");      
  Serial.print(now.day(), DEC); Serial.print('/');
  if(int(now.month()) < 10) Serial.print("0");
  Serial.print(now.month(), DEC); Serial.print("/");
  Serial.println(now.year(), DEC);
}