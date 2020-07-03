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
U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

void setup() {
  Serial.begin(9600);
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

  u8g2.begin();
}

void loop() {

  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_profont11_tf);
    char *line = "loc: ";
    u8g2.drawStr(0, 16, line);
    // u8g2.drawStr(0,16,"Hallo Welt");
  } while ( u8g2.nextPage() );


  while (gpss.available() > 0){
    gps.encode(gpss.read());
    if (gps.location.isUpdated()){
      sendInfoToSerial();
      displayInfo();
    }
  }
  if (millis() > 5000 && gps.charsProcessed() < 10)
  {
    Serial.println(F("No GPS detected: check wiring."));
    while(true);
  }
}





void displayInfo(){
  u8g2.firstPage();
  do {
//    u8g2.clearBuffer();
    String buf;
    u8g2.setFont(u8g2_font_profont11_tf);
    buf += "loc: ";
    buf += String(gps.location.lat(), 5);
    buf += ", ";
    buf += String(gps.location.lng(), 5);
    u8g2.drawStr(0,7,"buf");
    u8g2.drawLine(0,18,128,18);
  } while ( u8g2.nextPage() );
}

void sendInfoToSerial(){
  // Location
  Serial.print(F("Location: ")); 
  if (gps.location.isValid())
  {
    Serial.print(gps.location.lat(), 5);
    Serial.print(F(","));
    Serial.print(gps.location.lng(), 5);
  }
  else
  {
    Serial.print(F("INVALID"));
  }
  // Altitude
  Serial.print(F("  Altitude: ")); 
  if (gps.location.isValid())
  {
    Serial.print(gps.altitude.meters(), 0);
  }
  else
  {
    Serial.print(F("INVALID"));
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
    Serial.print(F("INVALID"));
  }
  // Course
  Serial.print(F("  Course: ")); 
  if (gps.location.isValid())
  {
    Serial.print(gps.course.deg(), 0);
  }
  else
  {
    Serial.print(F("INVALID"));
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
    Serial.print(F("INVALID"));
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
    Serial.print(F("INVALID"));
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