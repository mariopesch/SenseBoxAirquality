/*SenseBox AirQuality:
Sensors:  - GPS to D2 (GPS not used in this version)  
          - OLED I2C
          - HDC100X I2C
          - BMP280 I2C
          - RTC RV8523 I2C
          - Shinyei PPD42 18/19
          - MH-Z14 RXpin 68 TXpin 69 see Datasheet for more informations
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
#include <SdFat.h>
#include <SPI.h>
#include "BMP280.h"
#include <RV8523.h>

SdFat SD;

RV8523 rtc;


static const int RXPin = 68, TXPin = 69;
static const uint32_t GPSBaud = 9600;

// The TinyGPS++ object
TinyGPSPlus gps;

// The serial connection to the GPS device
SoftwareSerial ss(RXPin, TXPin);

SoftwareSerial mySerial(66, 67);

//I2C settings for the HDC100X
HDC100X HDC1(0x43);
//Settings for SD-Shield
#define serialSD 4

File file;
String nameOfFile;

boolean firstTime = true;

//const int switchPin = 31;
//int switchState = 0;

byte cmd[9] = {0xFF,0x01,0x86,0x00,0x00,0x00,0x00,0x00,0x79}; 
char response[9]; 
String ppmString = " ";

//define stuff for the Dust Sensor
unsigned long starttime;

unsigned long triggerOnP1;
unsigned long triggerOffP1;
unsigned long pulseLengthP1;
unsigned long durationP1;
boolean valP1 = HIGH;
boolean triggerP1 = false;

unsigned long triggerOnP2;
unsigned long triggerOffP2;
unsigned long pulseLengthP2;
unsigned long durationP2;
boolean valP2 = HIGH;
boolean triggerP2 = false;

float ratioP1 = 0;
float ratioP2 = 0;
unsigned long sampletime_ms = 30000;
float countP1;
float countP2;
float concSmall;
int ppm;
int cases = 1;

//generating strings
char tString [20];
char hString [20];
char co2String [20];
char pString [20];
char pm25String [20];
//empty outputstring
String output = "";

//BMP280
BMP280 bmp;
#define P0 1013.25
double T,P;

//RTC
uint8_t sec, min, hour, day, month;
uint16_t year;

void setup()
{

  mySerial.begin(9600);
  Wire.begin();
  Serial.begin(9600);
  //ss.begin(GPSBaud);
  HDC1.begin(HDC100X_TEMP_HUMI,HDC100X_14BIT,HDC100X_14BIT,DISABLE);
  bmp.begin();
  bmp.setOversampling(4);
  
   //display
    SeeedGrayOled.init();             //initialize SEEED OLED display
    SeeedGrayOled.clearDisplay(); //Clear Display.
    SeeedGrayOled.setNormalDisplay(); //Set Normal Display Mode
    SeeedGrayOled.setVerticalMode();  // Set to vertical mode for displaying text
    SeeedGrayOled.setTextXY(1,0);
    SeeedGrayOled.putString("SenseBox:AQ");
 
 SD.begin(serialSD);
 
//pinMode(switchPin, INPUT);
pinMode(19, INPUT);
pinMode(18, INPUT);
pinMode(33, OUTPUT);

  //start RTC
  rtc.start();

  //When the power source is removed, the RTC will keep the time.
  rtc.batterySwitchOverOn(); //battery switch over on


}

void loop()
{
  
  switch (cases){
    case 1:
      digitalWrite(33, HIGH);
      Serial.println("case 1");
      output = "";
      dust();
      break;
    case 2:
      co2();
      break;
    case 3:
      humitemp();
      break;
    case 4:
    baromter();
    break;
    case 5:
      displayoutput();
      break; 
    case 6:
        //switchState = digitalRead(switchPin);
        //Prepare filename and open file on SD
        char charFileName[nameOfFile.length()+1]; 
        nameOfFile.toCharArray(charFileName, sizeof(charFileName));
        file = SD.open(charFileName, FILE_WRITE);
        getrtc();
        writeData(output); //write the datastring to the sd
        break;
      
  }
    if (cases == 6) {
      cases = 1;
    } else cases++;
 
}


void dust(){
  do
  {
  valP1 = digitalRead(18);
  valP2 = digitalRead(19);
  
  if(valP1 == LOW && triggerP1 == false){
    triggerP1 = true;
    triggerOnP1 = micros();
  }
  
  if (valP1 == HIGH && triggerP1 == true){
      triggerOffP1 = micros();
      pulseLengthP1 = triggerOffP1 - triggerOnP1;
      durationP1 = durationP1 + pulseLengthP1;
      triggerP1 = false;
  }
  
    if(valP2 == LOW && triggerP2 == false){
    triggerP2 = true;
    triggerOnP2 = micros();
  }
  
    if (valP2 == HIGH && triggerP2 == true){
      triggerOffP2 = micros();
      pulseLengthP2 = triggerOffP2 - triggerOnP2;
      durationP2 = durationP2 + pulseLengthP2;
      triggerP2 = false;
  }
  
    
      if ((millis() - starttime) >= sampletime_ms) {
      
      ratioP1 = durationP1/(sampletime_ms*10.0);  // Integer percentage 0=>100
      ratioP2 = durationP2/(sampletime_ms*10.0);
      countP1 = 1.1*pow(ratioP1,3)-3.8*pow(ratioP1,2)+520*ratioP1+0.62;
      countP2 = 1.1*pow(ratioP2,3)-3.8*pow(ratioP2,2)+520*ratioP2+0.62;
      float PM10count = countP2;
      float PM25count = countP1 - countP2;
      
      // calculation the pm1.0 mass concentration for infos see:http://www.cleanair.org/sites/default/files/Drexel%20Air%20Monitoring_-_Final_Report_-_Team_19_0.pdf
      double r10 = 2.6*pow(10,-6);
      double pi = 3.14159;
      double vol10 = (4/3)*pi*pow(r10,3);
      double density = 1.65*pow(10,12);
      double mass10 = density*vol10;
      double K = 3531.5;
      float concLarge = (PM10count)*K*mass10;
      
      // calculation the pm2.5 mass concentration
      double r25 = 0.44*pow(10,-6);
      double vol25 = (4/3)*pi*pow(r25,3);
      double mass25 = density*vol25;
      float concSmall = (PM25count)*K*mass25;
      durationP1 = 0;
      durationP2 = 0;
      Serial.println(concSmall);
      dtostrf (concSmall, 5,2,pm25String);
      output +=pm25String;
      output +=";";
      }
}
while ((millis() - starttime) <= sampletime_ms); //this is an important step to make sure the measurement time for the particle count is right
starttime = millis();
 
}

void co2(){
      mySerial.listen();
      mySerial.write(cmd,9);
      mySerial.readBytes(response, 9);
      int responseHigh = (int) response[2];
      int responseLow = (int) response[3];
      int ppm = (256*responseHigh)+responseLow;

      ppmString = String(ppm); //int to string
      Serial.print("PPM ");
      Serial.println(ppm);
      dtostrf (ppm, 4,0,co2String);
      output +=co2String;
      output +=";";
      }

void humitemp(){
        //float to string conversation
        dtostrf(HDC1.getTemp(), 4, 2, tString);
        dtostrf(HDC1.getHumi(), 4, 2, hString);
        Serial.println(tString);
        Serial.println(hString);
        output +=tString;
        output +=";";
        output +=hString;
        output +=";";
        
        
}

void baromter(){

char result = bmp.startMeasurment();
result = bmp.getTemperatureAndPressure(T,P);
dtostrf(P, 7, 2, pString);
      output += pString;
      output +=";";
      
}
void displayoutput(){
  SeeedGrayOled.clearDisplay();
  SeeedGrayOled.setGrayLevel(15); //Set Grayscale level. Any number between 0 - 15.
  SeeedGrayOled.setTextXY(1, 0); //set Cursor to ith line, 0th column
  SeeedGrayOled.putString("CO2-Konz:");
  SeeedGrayOled.setTextXY(2,0);
  SeeedGrayOled.putString(co2String);
  SeeedGrayOled.putString(" ppm");
  SeeedGrayOled.setTextXY(3,0);
  SeeedGrayOled.putString("PM2.5");
  SeeedGrayOled.setTextXY(4,0);
  SeeedGrayOled.putString(pm25String);
  SeeedGrayOled.setTextXY(5,0);
  SeeedGrayOled.putString("Temperatur");
  SeeedGrayOled.setTextXY(6,0);
  SeeedGrayOled.putString(tString);
  SeeedGrayOled.putString("C");
  SeeedGrayOled.setTextXY(7,0);
  SeeedGrayOled.putString("Humidity:");
  SeeedGrayOled.setTextXY(8,0);
  SeeedGrayOled.putString(hString);
  SeeedGrayOled.putString("%");
  SeeedGrayOled.setTextXY(9,0);
  SeeedGrayOled.putString(pString);
  SeeedGrayOled.putString("hPa");

}

String generateFileName(String boardID)
{
  String fileName = String();
  unsigned int filenumber = 1;
  boolean isFilenameExisting;
  do{
    fileName = boardID;
    fileName += "-";
    fileName += filenumber;
    fileName += ".csv";
    Serial.println(fileName);
    char charFileName[fileName.length() + 1];
    fileName.toCharArray(charFileName, sizeof(charFileName));
   
    filenumber++;

    isFilenameExisting = SD.exists(charFileName);
  }while(isFilenameExisting); 
  
   Serial.print("Generated filename: ");
   Serial.println(fileName);
  
   return fileName; 
}

void getrtc(){

  //get time from RTC
  rtc.get(&sec, &min, &hour, &day, &month, &year);
  output += day;
  output += ".";
  output += month;
  output += ".";
  output += year;
  output += ";";
  output += hour;
  output += ":";
  output += min;
  output += ":";
  output += sec;
}  

// write the data to the output channels
void writeData(String txt) {
  //// if first time a text should be outputted, output the column names first
  if (firstTime) {
      
      nameOfFile = generateFileName("a01");
      char charFileName[nameOfFile.length()+1];
      nameOfFile.toCharArray(charFileName, sizeof(charFileName));
      firstTime = false;
      file = SD.open(charFileName, FILE_WRITE);  
      if (file) {
        file.println("PM2.5;CO2;Temperature; Humidity; Pressure; Date; Time"); //write the CSV Header to the file
        file.close();
        }
  }
  // serial output
  Serial.println(txt);
  // sd card output
  file.println(txt);
  file.flush();
  SeeedGrayOled.setTextXY(10,0);
  SeeedGrayOled.putString("Save");
  SeeedGrayOled.setTextXY(10,4);
  SeeedGrayOled.putNumber(hour);
  SeeedGrayOled.putString(":");
  SeeedGrayOled.putNumber(min);
  SeeedGrayOled.putString(":");
  SeeedGrayOled.putNumber(sec);
  
}


