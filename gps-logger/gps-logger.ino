#include <Arduino.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <U8g2lib.h>
//#include <SPI.h>
//#include <Arduino.h>
//#include <Wire.h>

// GPS
static const int RXPin = 2, TXPin = 3; //GPS communication
static const uint32_t GPSBaud = 9600;
SoftwareSerial gpss(RXPin, TXPin);

// TinyGPS++ object
TinyGPSPlus gps;

// Display
// U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
#include "SSD1306Ascii.h"
#include "SSD1306AsciiAvrI2c.h"

// 0X3C+SA0 - 0x3C or 0x3D
#define I2C_ADDRESS 0x3C

// Define proper RST_PIN if required.
#define RST_PIN -1

SSD1306AsciiAvrI2c oled;


void setup() {
  Serial.begin(19200);
  Serial.println("---------------");
  Serial.println("GPS-LOGGER");
  Serial.println("(c)2020 Patrick Itten, www.patrickitten.ch");
  Serial.println("---------------");

  while (!Serial) {
    ; // wait for serial port to connect. Needed for Native USB only
  }
  Serial.println("USB Connection established.");

  gpss.begin(GPSBaud);
  while (gpss.available() == 0){
    ; // wait for gps serial port to connect.
  }
  Serial.println("Connection to GPS established.");
  Serial.println("---------------");

  #if RST_PIN >= 0
    oled.begin(&Adafruit128x64, I2C_ADDRESS, RST_PIN);
  #else // RST_PIN >= 0
    oled.begin(&Adafruit128x64, I2C_ADDRESS);
  #endif // RST_PIN >= 0
    // Call oled.setI2cClock(frequency) to change from the default frequency.
  oled.setFont(System5x7);
  oled.clear();
  oled.println("GPS LOGGER");
  oled.println("Initializing...");
}

void loop() {

  while (gpss.available() > 0){
    gps.encode(gpss.read());
    sendInfoToSerial();
    // if (gps.location.isUpdated()){
      displayInfo();
    // }
    delay(2000);
  }
  if (millis() > 5000 && gps.charsProcessed() < 10)
  {
    Serial.println(F("No GPS detected: check wiring."));
    while(true);
  }
}





void displayInfo(){
  oled.clear();
  oled.print("lat: ");
  oled.print(gps.location.lat(), 5);
  oled.println();
  oled.print("lng: ");
  oled.print(gps.location.lng(), 5);
  oled.println();
  oled.print("alt: ");
  oled.print(gps.altitude.meters(), 0);
  oled.print(" m");
  oled.println();
  oled.print("spd: ");
  oled.print(gps.speed.kmph(), 2);
  oled.print(" kmh ");
  oled.print(gps.speed.knots(), 2);
  oled.print(" kt");
  oled.println();
  oled.print("crs: ");
  oled.print(gps.course.deg(), 0);
  oled.println();
  oled.print("dat: ");
  oled.print(gps.date.month());
  oled.print(F("/"));
  oled.print(gps.date.day());
  oled.print(F("/"));
  oled.print(gps.date.year());
  oled.println();
  oled.print("tme: ");
  if (gps.time.hour() < 10) oled.print(F("0"));
  oled.print(gps.time.hour());
  oled.print(F(":"));
  if (gps.time.minute() < 10) oled.print(F("0"));
  oled.print(gps.time.minute());
  oled.print(F(":"));
  if (gps.time.second() < 10) oled.print(F("0"));
  oled.print(gps.time.second());
  oled.print(F("."));
  if (gps.time.centisecond() < 10) oled.print(F("0"));
  oled.print(gps.time.centisecond());
  oled.println();
  oled.print("sat: ");
  oled.print(gps.satellites.value());

}

void sendInfoToSerial(){
  // Satellites
  Serial.print(F("Satellites: ")); 
  Serial.print(gps.satellites.value());
  
  // Location
  Serial.print(F("  Location: ")); 
  if (gps.location.isValid())
  {
    Serial.print(gps.location.lat(), 5);
    Serial.print(F(","));
    Serial.print(gps.location.lng(), 5);
  }
  else
  {
    Serial.print(F("n.A."));
  }
  // Altitude
  Serial.print(F("  Altitude: ")); 
  if (gps.location.isValid())
  {
    Serial.print(gps.altitude.meters(), 0);
  }
  else
  {
    Serial.print(F("n.A."));
  }
  // Speed
  Serial.print(F("  Speed: ")); 
  if (gps.location.isValid())
  {
    Serial.print(gps.speed.kmph(), 2);
    Serial.print(" km/h ");
    Serial.print(gps.speed.knots(), 2);
    Serial.print(" kn ");
  }
  else
  {
    Serial.print(F("n.A."));
  }
  // Course
  Serial.print(F("  Course: ")); 
  if (gps.location.isValid())
  {
    Serial.print(gps.course.deg(), 0);
  }
  else
  {
    Serial.print(F("n.A."));
  }
  // Date/Time
  Serial.print(F("  Date/Time: "));
  if (gps.date.isValid())
  {
    Serial.print(gps.date.month());
    Serial.print(F("/"));
    Serial.print(gps.date.day());
    Serial.print(F("/"));
    Serial.print(gps.date.year());
  }
  else
  {
    Serial.print(F("n.A."));
  }
  Serial.print(F(" "));
  if (gps.time.isValid())
  {
    if (gps.time.hour() < 10) Serial.print(F("0"));
    Serial.print(gps.time.hour());
    Serial.print(F(":"));
    if (gps.time.minute() < 10) Serial.print(F("0"));
    Serial.print(gps.time.minute());
    Serial.print(F(":"));
    if (gps.time.second() < 10) Serial.print(F("0"));
    Serial.print(gps.time.second());
    Serial.print(F("."));
    if (gps.time.centisecond() < 10) Serial.print(F("0"));
    Serial.print(gps.time.centisecond());
  }
  else
  {
    Serial.print(F("n.A."));
  }

  Serial.println();
}


// delay for a good reception
static void smartDelay(unsigned long ms)
{
  unsigned long start = millis();
  do
  {
    while (gpss.available())
      gps.encode(gpss.read());
  } while (millis() - start < ms);
}