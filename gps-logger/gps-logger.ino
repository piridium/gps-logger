#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <SSD1306Ascii.h>
#include <SSD1306AsciiAvrI2c.h>
#include <SPI.h>
#include <SD.h>

#define LOG_FREQ 10000
#define UPD_FREQ 5000

// Display
// 0X3C+SA0 - 0x3C or 0x3D
#define DISP_ADDRESS 0x3C
// Define proper RST_PIN if required.
#define RST_PIN -1
SSD1306AsciiAvrI2c oled;

// GPS
#define GPSBaud 9600
// The TinyGPS++ object
TinyGPSPlus gps;
// The serial connection to the GPS device
SoftwareSerial ss(2, 3); // (RX,TX)

// SD
// set up variables using the SD utility library functions:
// Sd2Card card;
// SdVolume volume;
// SdFile root;
#define chipSelect 4
bool sdFailure = false;
unsigned long logTime;

// Buffer
// global buffer for datapoints
char buffer[42]; // longest buffer seems to be: "<trkpt lat="047.575336" lon="009.368504">" : 41 (+1 for NULL terminator)
// int bufferSize = 0;

// GPX
// #define GPX_HEADER ""

void setup()
{
  // DISPLAY
  if (RST_PIN >= 0){
    oled.begin(&Adafruit128x64, DISP_ADDRESS, RST_PIN);
  } else {
    oled.begin(&Adafruit128x64, DISP_ADDRESS);
  } // Call oled.setI2cClock(frequency) to change from the default frequency.
  oled.setFont(font5x7);
  oled.clear();
  oled.println(F("GPS LOGGER"));
  oled.println(F("(c)2020 Patrick Itten"));
  oled.println();
  oled.println(F("initializing..."));

  // SERIAL MONITOR
  Serial.begin(115200);
  while (!Serial);  // wait for serial port to connect. Needed for native USB
  Serial.println(F("GPS-LOGGER"));
  Serial.println(F("(c)2020 by Patrick Itten"));
  Serial.println();

  // SD
  Serial.println(F("Initializing SD card..."));
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println(F("Card failed, or not present"));
    oled.println(F("...no card found."));
    sdFailure = true;
  } else {
    Serial.println(F("card initialized."));
    oled.println(F("...card initialized."));
  }
  Serial.println();
  delay(3000);
  oled.clear();

  // GPS
  ss.begin(GPSBaud);

}

void loop()
{

  if (gps.location.isUpdated()){
    showInfo(true); // true: new position
    if (millis() - logTime > LOG_FREQ ){
      writeLog();
      logTime = millis();
    }
  } else {
    showInfo(false); // false: same position
  }
  smartDelay( UPD_FREQ ); // Update frequency

  if (millis() > 5000 && gps.charsProcessed() < 10)
    Serial.println(F("No GPS data received: check wiring"));
}

void showInfo(bool updated){

  Serial.println();

  if (updated){
    oled.clear();
  } else {
    oled.clear(0,oled.displayWidth(), 0, 2);
  }

  // Date
  loadBufferDate(gps.date, "day: ");
  Serial.print(buffer);
  oled.println(buffer);

  // Time
  loadBufferTime(gps.time, "utc: ");
  Serial.print(buffer);
  oled.println(buffer);

  // Satellites
  loadBuffer(gps.satellites.value(), gps.satellites.isValid(), "sat: ", "", 1, 0);
  Serial.print(buffer);
  oled.println(buffer);

  if (updated){
    // HDOP
    loadBuffer(gps.hdop.hdop(), gps.hdop.isValid(), "hdop: ", "", 3, 1);
    Serial.print(buffer);

    // Latitude
    loadBuffer(gps.location.lat(), gps.location.isValid(), "lat: ", "", 3, 5);
    Serial.print(buffer);
    oled.println(buffer);

    // Longitude
    loadBuffer(gps.location.lng(), gps.location.isValid(), "lng: ", "", 3, 5);
    Serial.print(buffer);
    oled.println(buffer);

    // Altitude
    loadBuffer(gps.altitude.meters(), gps.altitude.isValid(), "alt: ", "m", 3, 1);
    Serial.print(buffer);
    oled.println(buffer);

    // Speed
    loadBuffer(gps.speed.kmph(), gps.speed.isValid(), "spd: ", "kmh", 3, 1);
    Serial.print(buffer);
    oled.print(buffer);
    loadBuffer(gps.speed.knots(), gps.speed.isValid(), "", "kt", 3, 1);
    Serial.print(buffer);
    oled.println(buffer);

    // Course
    loadBuffer(gps.course.deg(), gps.course.isValid(), "crs: ", "", 1, 0);
    Serial.print(buffer);
    oled.print(buffer);
    
    // Course Cardinal
    strcpy (buffer, gps.course.isValid() ? TinyGPSPlus::cardinal(gps.course.deg()) : "***");
    strcat (buffer, " ");
    Serial.print(buffer);
    oled.print(buffer);
    //printMaxBufferSize();
    // Serial.print(gps.course.isValid() ? TinyGPSPlus::cardinal(gps.course.deg()) : "** ");

    // Fix age
    loadBuffer(gps.location.age(), gps.location.isValid(), "age: ", "ms", 3, 0);
    Serial.print(buffer);

    // printInt(gps.charsProcessed(), true, 6);
    // printInt(gps.sentencesWithFix(), true, 10);
    // printInt(gps.failedChecksum(), true, 9);
  }
  if (sdFailure){
    oled.print("   *SDerr");
  }
}

void writeLog(){
  char filename[16];
  sprintf(filename, "%02d%02d%02d.gpx", gps.date.year(), gps.date.month(), gps.date.day());
  File dataFile = SD.open(filename, FILE_WRITE);
  if (dataFile) {
    // char datarow[128];
    // sprintf(datarow, "<trkpt lat=\"%.6f\" lon=\"%.6f\"></trkpt>", gps.location.lat(), gps.location.lng());
//    dataFile.println(buffer);
    char buf[32];
    // opening
    strcpy (buffer, "<trkpt lat=\"");
    dtostrf (gps.location.lat(), 0, 6, buf);
    strcat (buffer, buf);
    strcat (buffer, "\" lon=\"");
    dtostrf (gps.location.lng(), 0, 6, buf);
    strcat (buffer, buf);
    strcat (buffer, "\">");
    dataFile.println(buffer);
    //printMaxBufferSize();
    
    // time
    strcpy (buffer, "<time>");
    sprintf(buf, "%02d-%02d-%02dT%02d:%02d:%02dZ", gps.date.year(), gps.date.month(), gps.date.day(), gps.time.hour(), gps.time.minute(), gps.time.second());
    strcat (buffer, buf);
    strcat (buffer, "</time>");
    dataFile.println(buffer);
    //printMaxBufferSize();
    // elevation
    gpxBuffer("ele", gps.altitude.meters(), 0);
    dataFile.println(buffer);
    // course
    gpxBuffer("course", gps.course.deg(), 0);
    dataFile.println(buffer);
    // speed
    gpxBuffer("speed", gps.speed.mps(), 1);
    dataFile.println(buffer);
    // sat
    gpxBuffer("sat", gps.satellites.value(), 0);
    dataFile.println(buffer);
    // hdop
    gpxBuffer("hdop", gps.hdop.hdop(), 1);
    dataFile.println(buffer);
    // closing
    dataFile.println("</trkpt>");

    dataFile.close();
    sdFailure = false;
    Serial.print("log written");
  }
  else {
    Serial.println(F("error opening logfile"));
    sdFailure = true;
  }
}

static void smartDelay(unsigned long ms)
{
  // This custom version of delay() ensures that the gps object is being "fed".
  unsigned long start = millis();
  do 
  {
    while (ss.available())
      gps.encode(ss.read());
  } while (millis() - start < ms);
}

static void loadBuffer(float val, bool valid, char prefix[], char suffix[], int len, int prec) // len: minimal length of number part, prec: precision of number part
{
  strcpy (buffer, prefix);

  if (!valid) {
    while (len-- > 0)
      strcat (buffer, "*");
  } else {
    char buf[32];
    dtostrf (val, len, prec, buf);
    strcat (buffer, buf);
    strcat (buffer, suffix);
  }

  strcat (buffer, " ");
  //printMaxBufferSize();
  smartDelay(0);
}

static void gpxBuffer(char tag[], float val, int prec)
{
  //open
  strcpy (buffer, "<");
  strcat (buffer, tag);
  strcat (buffer, ">");

  char buf[32];
  dtostrf (val, 0, prec, buf);
  strcat (buffer, buf);

  //close
  strcat (buffer, "</");
  strcat (buffer, tag);
  strcat (buffer, ">");
  
  // printMaxBufferSize();
}

static void loadBufferDate(TinyGPSDate &d, char prefix[])
{
  strcpy (buffer, prefix);
  if (!d.isValid())
  {
    strcat(buffer, "********** ");
  }
  else
  {
    char sz[32];
    sprintf(sz, "%02d-%02d-%02d ", d.month(), d.day(), d.year());
    strcat(buffer, sz);
  }
  //printMaxBufferSize();
}

static void loadBufferTime(TinyGPSTime &t, char prefix[])
{
  strcpy (buffer, prefix);
  if (!t.isValid())
  {
    strcat(buffer, "******** ");
  }
  else
  {
    char sz[32];
    sprintf(sz, "%02d:%02d:%02d ", t.hour(), t.minute(), t.second());
    strcat(buffer, sz);
  }
  // printInt(d.age(), d.isValid(), 5);
  smartDelay(0);
}

// static void printMaxBufferSize(){
//   if (strlen(buffer) > bufferSize){
//     Serial.println(F("********************"));
//     Serial.print("length of \"");
//     Serial.print(buffer);
//     Serial.print("\" : ");
//     Serial.println(strlen(buffer));
//     Serial.println(F("********************"));

//     bufferSize = strlen(buffer);
//   }
// }
