#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <SSD1306Ascii.h>
#include <SSD1306AsciiAvrI2c.h>
#include <SPI.h>
#include <SD.h>

// Display
// 0X3C+SA0 - 0x3C or 0x3D
#define DISP_ADDRESS 0x3C
// Define proper RST_PIN if required.
#define RST_PIN -1
SSD1306AsciiAvrI2c oled;

// GPS
static const int RXPin = 2, TXPin = 3;
static const uint32_t GPSBaud = 9600;
// The TinyGPS++ object
TinyGPSPlus gps;
// The serial connection to the GPS device
SoftwareSerial ss(RXPin, TXPin);

// SD
// set up variables using the SD utility library functions:
Sd2Card card;
SdVolume volume;
SdFile root;
const int chipSelect = 4;

void setup()
{
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println(F("GPS-LOGGER"));
  Serial.print(F("TinyGPS++ library v. ")); Serial.println(TinyGPSPlus::libraryVersion());
  Serial.println(F("(c)2020 by Patrick Itten"));
  Serial.println();

  // SD
  Serial.print("Initializing SD card...");
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    while (1);
  }
  Serial.println("card initialized.");
  Serial.println();

  // HEADER
  Serial.println(F("Sats HDOP  Latitude  Longitude  Fix  Date       Time     Date  Alt     Course Speed Card  Chars Sentences Checksum"));
  Serial.println(F("           (deg)     (deg)      Age             UTC      Age   (m)     --- from GPS ----  RX    RX        Fail"));
  Serial.println(F("--------------------------------------------------------------------------------------------------------------------------------------"));

  // GPS
  ss.begin(GPSBaud);

  // DISPLAY
  #if RST_PIN >= 0
    oled.begin(&Adafruit128x64, DISP_ADDRESS, RST_PIN);
  #else // RST_PIN >= 0
    oled.begin(&Adafruit128x64, DISP_ADDRESS);
  #endif // RST_PIN >= 0
    // Call oled.setI2cClock(frequency) to change from the default frequency.
  oled.setFont(System5x7);
  oled.clear();
  oled.println("GPS LOGGER");
  oled.println("initializing...");
}

void loop()
{
  serialInfo();

  if (gps.location.isUpdated()){
    displayInfo();
    writeLog();
  }
  
  smartDelay(1000);

  if (millis() > 5000 && gps.charsProcessed() < 10)
    Serial.println(F("No GPS data received: check wiring"));
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
  oled.print("utc: ");
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

void writeLog(){
  String dataString = "";



  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  File dataFile = SD.open("datalog.txt", FILE_WRITE);

  // if the file is available, write to it:
  if (dataFile) {
    dataFile.println(dataString);
    dataFile.close();
  }
  // if the file isn't open, pop up an error:
  else {
    Serial.println("error opening datalog.txt");
  }
}

void serialInfo(){

  printInt(gps.satellites.value(), gps.satellites.isValid(), 5);
  printFloat(gps.hdop.hdop(), gps.hdop.isValid(), 6, 1);
  printFloat(gps.location.lat(), gps.location.isValid(), 10, 6);
  printFloat(gps.location.lng(), gps.location.isValid(), 11, 6);
  printInt(gps.location.age(), gps.location.isValid(), 6);
  printDateTime(gps.date, gps.time);
  printFloat(gps.altitude.meters(), gps.altitude.isValid(), 8 , 2);
  printFloat(gps.course.deg(), gps.course.isValid(), 7, 2);
  printFloat(gps.speed.kmph(), gps.speed.isValid(), 6, 2);
  printStr(gps.course.isValid() ? TinyGPSPlus::cardinal(gps.course.deg()) : "*** ", 6);

  printInt(gps.charsProcessed(), true, 6);
  printInt(gps.sentencesWithFix(), true, 10);
  printInt(gps.failedChecksum(), true, 9);
  Serial.println();
}

// This custom version of delay() ensures that the gps object
// is being "fed".
static void smartDelay(unsigned long ms)
{
  unsigned long start = millis();
  do 
  {
    while (ss.available())
      gps.encode(ss.read());
  } while (millis() - start < ms);
}

static void printFloat(float val, bool valid, int len, int prec)
{
  if (!valid)
  {
    while (len-- > 1)
      Serial.print('*');
    Serial.print(' ');
  }
  else
  {
    Serial.print(val, prec);
    int vi = abs((int)val);
    int flen = prec + (val < 0.0 ? 2 : 1); // . and -
    flen += vi >= 1000 ? 4 : vi >= 100 ? 3 : vi >= 10 ? 2 : 1;
    for (int i=flen; i<len; ++i)
      Serial.print(' ');
  }
  smartDelay(0);
}

static void printInt(unsigned long val, bool valid, int len)
{
  char sz[32] = "*****************";
  if (valid)
    sprintf(sz, "%ld", val);
  sz[len] = 0;
  for (int i=strlen(sz); i<len; ++i)
    sz[i] = ' ';
  if (len > 0) 
    sz[len-1] = ' ';
  Serial.print(sz);
  smartDelay(0);
}

static void printDateTime(TinyGPSDate &d, TinyGPSTime &t)
{
  if (!d.isValid())
  {
    Serial.print(F("********** "));
  }
  else
  {
    char sz[32];
    sprintf(sz, "%02d/%02d/%02d ", d.month(), d.day(), d.year());
    Serial.print(sz);
  }
  
  if (!t.isValid())
  {
    Serial.print(F("******** "));
  }
  else
  {
    char sz[32];
    sprintf(sz, "%02d:%02d:%02d ", t.hour(), t.minute(), t.second());
    Serial.print(sz);
  }

  printInt(d.age(), d.isValid(), 5);
  smartDelay(0);
}

static void printStr(const char *str, int len)
{
  int slen = strlen(str);
  for (int i=0; i<len; ++i)
    Serial.print(i<slen ? str[i] : ' ');
  smartDelay(0);
}