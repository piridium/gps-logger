/**
 * GPS-Logger by Patrick Itten (c)2020
 * based on the code of Mark Fickett : https://github.com/markfickett/gpslogger
 */

#include <SoftwareSerial.h>
#include <SdFat.h>
#include <TinyGPS.h>
#include <SSD1306AsciiAvrI2c.h>

#define SAMPLE_INTERVAL_MS 1000

#define PIN_STATUS_LED 13

#define PIN_RX_FROM_GPS 2
#define PIN_TX_TO_GPS 3
#define PIN_SD_CHIP_SELECT 4
#define PIN_SPI_CHIP_SELECT_REQUIRED 4
// The SD library requires, for SPI communication:
// 11 MOSI (master/Arduino output, slave/SD input)
// 12 MISO (master input, slave output)
// 13 CLK (clock)
#define GPS_BAUD 9600

// Display
// 0X3C+SA0 - 0x3C or 0x3D
#define DISP_ADDRESS 0x3C
// Define proper RST_PIN if required.
#define RST_PIN -1
SSD1306AsciiAvrI2c oled;

// Seek to fileSize + this position before writing track points.
#define SEEK_TRKPT_BACKWARDS -24
#define GPX_EPILOGUE "\t</trkseg></trk>\n</gpx>\n"
#define LATLON_PREC 6

TinyGPS gps;
SoftwareSerial nss(PIN_RX_FROM_GPS, PIN_TX_TO_GPS);

SdFat sd;
SdFile gpxFile;

// General-purpose text buffer used in formatting.
char buf[32];


struct GpsSample {
  float lat_deg,
        lon_deg;
  float altitude_m;

  int satellites;
  int hdop_hundredths;

  // How many ms, according to millis(), since the last position data was read
  // from the GPS.
  unsigned long fix_age_ms;

  float speed_mps;
  float speed_kmh;
  float speed_kts;
  float course_deg;

  // How many ms, according to millis(), since the last datetime data was read
  // from the GPS.
  unsigned long datetime_fix_age_ms;

  int year;
  byte month,
       day,
       hour,
       minute,
       second,
       hundredths;
};
// The latest sample read from the GPS.
struct GpsSample sample;

void setup() {
  // DISPLAY
  if (RST_PIN >= 0){
    oled.begin(&Adafruit128x64, DISP_ADDRESS, RST_PIN);
  } else {
    oled.begin(&Adafruit128x64, DISP_ADDRESS);
  } // Call oled.setI2cClock(frequency) to change from the default frequency.
  oled.setFont(font5x7);
  oled.clear();
  oled.println(F("GPS LOGGER V2\n(c)2020 Patrick Itten\n\ninitializing..."));

  pinMode(PIN_STATUS_LED, OUTPUT);
  digitalWrite(PIN_STATUS_LED, HIGH);
  Serial.begin(115200);
  while (!Serial);  // wait for serial port to connect. Needed for native USB
  setUpSd();
  getFirstGpsSample();
  startFilesOnSdNoSync();
  delay(5000);
  digitalWrite(PIN_STATUS_LED, LOW);
  oled.clear();
}

void loop() {
  readFromGpsUntilSampleTime();
  fillGpsSample(gps);
  if (sample.fix_age_ms <= SAMPLE_INTERVAL_MS) {
    // TODO: Write whenever there is new data (trust GPS is set at 1Hz).
    writeGpxSampleToSd();
    displayGpxSample();
  }
}

void setUpSd() {
  if (PIN_SD_CHIP_SELECT != PIN_SPI_CHIP_SELECT_REQUIRED) {
    pinMode(PIN_SPI_CHIP_SELECT_REQUIRED, OUTPUT);
    oled.println(F("SD initialized"));
  }

  if (!sd.begin(PIN_SD_CHIP_SELECT, SPI_QUARTER_SPEED)) {
    sd.initErrorHalt();
  }
}

void getFirstGpsSample() {
  nss.begin(GPS_BAUD);

  while (true) {
    readFromGpsUntilSampleTime();
    fillGpsSample(gps);
    if (sample.fix_age_ms == TinyGPS::GPS_INVALID_AGE) {
      Serial.println(F("Waiting for location fix."));
    } else if (sample.fix_age_ms == TinyGPS::GPS_INVALID_AGE) {
      Serial.println(F("Waiting for datetime fix."));
    } else {
      Serial.println(F("Got GPS fix."));
      oled.println(F("Got GPS fix."));
      break;
    }
  }
}

static void readFromGpsUntilSampleTime() {
  unsigned long start = millis();
  // Process a sample from the GPS every second.
  while (millis() - start < SAMPLE_INTERVAL_MS) {
    readFromGpsSerial();
  }
}

static bool readFromGpsSerial() {
  while (nss.available()) {
    gps.encode(nss.read());
  }
}

static void startFilesOnSdNoSync() {
  // directory
  sprintf(
      buf,
      "%02d%02d%02d",
      sample.year,
      sample.month,
      sample.day);
  if (!sd.exists(buf)) {
    if (!sd.mkdir(buf)) {
      oled.println(F("ERR: dir not created."));
      sd.errorHalt("Creating log directory for today failed.");
    }
  }

  // SdFat will silently die if given a filename longer than "8.3"
  // (8 characters, a dot, and 3 file-extension characters).

  // GPX log
  openTimestampedFile(".gpx", gpxFile);
  gpxFile.print(F(
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<gpx version=\"1.0\" creator=\"Arduino GPS-Tracker\">\n"
    "\t<trk><name>Arduino GPS Track</name><trkseg>\n"));
  gpxFile.print(F(GPX_EPILOGUE));
}

static void openTimestampedFile(const char *shortSuffix, SdFile &file) {
  sprintf(
      buf,
      "%02d%02d%02d/%02d%02d%02d%s",
      sample.year,
      sample.month,
      sample.day,
      sample.hour,
      sample.minute,
      sample.second,
      shortSuffix);
  Serial.print(F("Starting file "));
  Serial.println(buf);
  oled.print(F("f:"));
  oled.println(buf);
  if (sd.exists(buf)) {
    Serial.println(F("warning: already exists, overwriting."));
    oled.println(F("warning: already exists, overwriting."));
  }
  if (!file.open(buf, O_CREAT | O_WRITE)) {
    sd.errorHalt();
  }
}

static void writeFloat(float v, SdFile &file, int precision) {
  obufstream ob(buf, sizeof(buf));
  ob << setprecision(precision) << v;
  file.print(buf);
}

static void writeFormattedSampleDatetime(SdFile &file) {
  sprintf(
      buf,
      "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ",
      sample.year,
      sample.month,
      sample.day,
      sample.hour,
      sample.minute,
      sample.second,
      sample.hundredths);
  file.print(buf);
}

static void writeGpxSampleToSd() {
  gpxFile.seekSet(gpxFile.fileSize() + SEEK_TRKPT_BACKWARDS);
  gpxFile.print(F("\t\t<trkpt "));

  gpxFile.print(F("lat=\""));
  writeFloat(sample.lat_deg, gpxFile, LATLON_PREC);
  gpxFile.print(F("\" lon=\""));
  writeFloat(sample.lon_deg, gpxFile, LATLON_PREC);
  gpxFile.print(F("\">"));

  gpxFile.print(F("<time>"));
  writeFormattedSampleDatetime(gpxFile);
  gpxFile.print(F("</time>"));

  if (sample.altitude_m != TinyGPS::GPS_INVALID_F_ALTITUDE) {
    gpxFile.print(F("<ele>")); // meters
    writeFloat(sample.altitude_m, gpxFile, 2 /* centimeter precision */);
    gpxFile.print(F("</ele>"));
  }

  if (sample.speed_mps != TinyGPS::GPS_INVALID_F_SPEED) {
    gpxFile.print(F("<speed>"));
    writeFloat(sample.speed_mps, gpxFile, 1);
    gpxFile.print(F("</speed>"));
  }
  if (sample.course_deg != TinyGPS::GPS_INVALID_F_ANGLE) {
    gpxFile.print(F("<course>"));
    writeFloat(sample.course_deg, gpxFile, 1);
    gpxFile.print(F("</course>"));
  }

  if (sample.satellites != TinyGPS::GPS_INVALID_SATELLITES) {
    gpxFile.print(F("<sat>"));
    gpxFile.print(sample.satellites);
    gpxFile.print(F("</sat>"));
  }
  if (sample.hdop_hundredths != TinyGPS::GPS_INVALID_HDOP) {
    gpxFile.print(F("<hdop>"));
    writeFloat(sample.hdop_hundredths / 100.0, gpxFile, 2);
    gpxFile.print(F("</hdop>"));
  }

  gpxFile.print(F("</trkpt>\n"));

  gpxFile.print(F(GPX_EPILOGUE));

  digitalWrite(PIN_STATUS_LED, HIGH);
  if (!gpxFile.sync() || gpxFile.getWriteError()) {
    Serial.println(F("SD sync/write error."));
    oled.clear();
    oled.println(F("Err: SD write error."));
    delay(1000);
  }
  digitalWrite(PIN_STATUS_LED, LOW);
}

static void displayGpxSample() {

  oled.clear();

  //Date
  sprintf(buf, "%04d-%02d-%02d", sample.year, sample.month, sample.day);
  oled.println(buf);

  // Time
  sprintf(buf, "%02d:%02d:%02d.%03d", sample.hour, sample.minute, sample.second, sample.hundredths);
  oled.println(buf);

  // Satellites
  addPrefixSuffix(sample.satellites, true, "sat: ", "", 1, 0);
  oled.println(buf);

  // Latitude
  addPrefixSuffix(sample.lat_deg, true, "lat: ", "", 3, LATLON_PREC);
  oled.println(buf);

  // Longitude
  addPrefixSuffix(sample.lon_deg, true, "lon: ", "", 3, LATLON_PREC);
  oled.println(buf);

  // Altitude
  addPrefixSuffix(sample.altitude_m, (sample.altitude_m != TinyGPS::GPS_INVALID_F_ALTITUDE), "alt: ", "m", 3, 1);
  oled.println(buf);

  // Speed
  addPrefixSuffix(sample.speed_kmh, (sample.speed_kmh != TinyGPS::GPS_INVALID_F_SPEED), "spd: ", "kmh", 3, 1);
  oled.print(buf);
  addPrefixSuffix(sample.speed_kts, (sample.speed_kts != TinyGPS::GPS_INVALID_F_SPEED), "", "kt", 3, 1);
  oled.println(buf);

  // Course
  addPrefixSuffix(sample.course_deg, (sample.course_deg != TinyGPS::GPS_INVALID_F_ANGLE), "crs: ", "", 1, 0);
  oled.print(buf);
  (sample.course_deg != TinyGPS::GPS_INVALID_F_ANGLE) ? oled.println(TinyGPS::cardinal(sample.course_deg)) : oled.println(F("***"));

}

static void fillGpsSample(TinyGPS &gps) {
  gps.f_get_position(
      &sample.lat_deg,
      &sample.lon_deg,
      &sample.fix_age_ms);
  sample.altitude_m = gps.f_altitude();

  sample.satellites = gps.satellites();
  sample.hdop_hundredths = gps.hdop();

  sample.course_deg = gps.f_course();
  sample.speed_mps = gps.f_speed_mps();
  sample.speed_kmh = gps.f_speed_kmph();
  sample.speed_kts = gps.f_speed_knots();

  gps.crack_datetime(
      &sample.year,
      &sample.month,
      &sample.day,
      &sample.hour,
      &sample.minute,
      &sample.second,
      &sample.hundredths,
      &sample.datetime_fix_age_ms);
}

static void addPrefixSuffix(float val, bool valid, char prefix[], char suffix[], int len, int prec) // len: minimal length of number part, prec: precision of number part
{
  strcpy (buf, prefix);

  if (!valid) {
    while (len-- > 0)
      strcat (buf, "*");
  } else {
    char buffer[32];
    dtostrf (val, len, prec, buffer);
    strcat (buf, buffer);
    strcat (buf, suffix);
  }

  strcat (buf, " ");
}