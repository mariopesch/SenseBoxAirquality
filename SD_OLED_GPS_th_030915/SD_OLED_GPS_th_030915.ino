/*SenseBox AirQuality:
Sensors:  - GPS to D2
          - OLED I2C
          - HDC100X I2C
          
GPS Baudrate needs to be set to 9600          
          */

#include <TinyGPS++.h>
#include <SoftwareSerial.h>
//display
#include <SeeedGrayOLED.h>
#include <Wire.h>
//temperature and humidity
#include <HDC100X.h>
//SD-Card
#include <SD.h>
#include <SPI.h>

static const int RXPin = 2, TXPin = 3;
static const uint32_t GPSBaud = 9600;

// The TinyGPS++ object
TinyGPSPlus gps;

// The serial connection to the GPS device
SoftwareSerial ss(RXPin, TXPin);

//I2C settings for the HDC100X
HDC100X HDC1(0x43);
//Settings for SD-Shield
#define serialSD 4

// column names for the csv output
#define csvColumns "latitude;longitude;Temperatur;Luftfeuchtigkeit"

File file;

boolean firstTime = true;

//const int switchPin = 8;
//int switchState = 0;

void setup()
{
  
  Wire.begin();
  Serial.begin(9600);
  ss.begin(GPSBaud);
  HDC1.begin(HDC100X_TEMP_HUMI,HDC100X_14BIT,HDC100X_14BIT,DISABLE);
  
    Serial.println(F("Sats HDOP Latitude   Longitude   Fix  Date       Time     Date Alt    Course Speed Card  Distance Course Card  Chars Sentences Checksum"));
    Serial.println(F("          (deg)      (deg)       Age                      Age  (m)    --- from GPS ----  ---- to London  ----  RX    RX        Fail"));
    Serial.println(F("---------------------------------------------------------------------------------------------------------------------------------------"));
  
   //display
    SeeedGrayOled.init();             //initialize SEEED OLED display
    SeeedGrayOled.clearDisplay();     //Clear Display.
    SeeedGrayOled.setNormalDisplay(); //Set Normal Display Mode
    SeeedGrayOled.setVerticalMode();  // Set to vertical mode for displaying text
 
 // open the file on the sd card
  file = SD.open("DATA1.CSV", O_CREAT | O_WRITE | O_APPEND);
  Serial.print("SD Card initialisieren");

//pinMode(switchPin, INPUT);
}


void loop()
{
  
  
  static const double LONDON_LAT = 51.508131, LONDON_LON = -0.128002;

  printInt(gps.satellites.value(), gps.satellites.isValid(), 5);
  printInt(gps.hdop.value(), gps.hdop.isValid(), 5);
  printFloat(gps.location.lat(), gps.location.isValid(), 11, 6);
  printFloat(gps.location.lng(), gps.location.isValid(), 12, 6);
  printInt(gps.location.age(), gps.location.isValid(), 5);
  printDateTime(gps.date, gps.time);
  printFloat(gps.altitude.meters(), gps.altitude.isValid(), 7, 2);
  printFloat(gps.course.deg(), gps.course.isValid(), 7, 2);
  printFloat(gps.speed.kmph(), gps.speed.isValid(), 6, 2);
  printStr(gps.course.isValid() ? TinyGPSPlus::cardinal(gps.course.value()) : "*** ", 6);

  unsigned long distanceKmToLondon =
    (unsigned long)TinyGPSPlus::distanceBetween(
      gps.location.lat(),
      gps.location.lng(),
      LONDON_LAT, 
      LONDON_LON) / 1000;
  printInt(distanceKmToLondon, gps.location.isValid(), 9);

  double courseToLondon =
    TinyGPSPlus::courseTo(
      gps.location.lat(),
      gps.location.lng(),
      LONDON_LAT, 
      LONDON_LON);

  printFloat(courseToLondon, gps.location.isValid(), 7, 2);

  const char *cardinalToLondon = TinyGPSPlus::cardinal(courseToLondon);

  printStr(gps.location.isValid() ? cardinalToLondon : "*** ", 6);

  printInt(gps.charsProcessed(), true, 6);
  printInt(gps.sentencesWithFix(), true, 10);
  printInt(gps.failedChecksum(), true, 9);
  Serial.println();
  
  smartDelay(1000);

  if (millis() > 5000 && gps.charsProcessed() < 10)
    Serial.println(F("No GPS data received: check wiring"));
    
          // float to string
        char latString[20];
        dtostrf(gps.location.lat(), 9, 6, latString);
        char lngString[20];
        dtostrf(gps.location.lng(), 9, 6, lngString);
        char tString [20];
        dtostrf(HDC1.getTemp(), 4, 2, tString);
        char hString [20];
        dtostrf(HDC1.getHumi(), 4, 2, hString);
        
        
  
  SeeedGrayOled.setGrayLevel(6); //Set Grayscale level. Any number between 0 - 15.
  SeeedGrayOled.setTextXY(1, 0); //set Cursor to ith line, 0th column
  SeeedGrayOled.putString("Latitude");
  SeeedGrayOled.setTextXY(2, 0);
  SeeedGrayOled.putString(latString);
  SeeedGrayOled.setTextXY(3,0);
  SeeedGrayOled.putString("Longitude");
  SeeedGrayOled.setTextXY(4,0);
  SeeedGrayOled.putString(lngString);
  SeeedGrayOled.setTextXY(5,0);
  SeeedGrayOled.putString("Temperatur");
  SeeedGrayOled.setTextXY(6,0);
  SeeedGrayOled.putString(tString);
  SeeedGrayOled.setTextXY(6,5);
  SeeedGrayOled.putString("°C");
  SeeedGrayOled.setTextXY(7,0);
  SeeedGrayOled.putString("Humidity:");
  SeeedGrayOled.setTextXY(8,0);
  SeeedGrayOled.putString(hString);

  // build one csv output string
        String output = "";
        output += latString;
        output += ";";
        output += lngString;
        output += ";";
        output += tString;
        output += ";";
        output += hString;

        // write the data to the output channels
        writeData(output);
        
//  switchState = digitalRead(switchPin);
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

// write the data to the output channels
void writeData(String txt) {
  //// if first time a text should be outputted, output the column names first
  if (firstTime) {

    // set the first time variable to false
    firstTime = false;

    // serial output
    Serial.println(csvColumns);

    // sd card output
    file.println(csvColumns);
  }

  //// output the text

  // serial output
  Serial.println(txt);

  // sd card output
  file.println(txt);
  file.flush();
}
