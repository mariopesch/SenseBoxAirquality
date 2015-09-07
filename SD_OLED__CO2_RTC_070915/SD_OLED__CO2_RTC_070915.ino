/*SenseBox AirQuality:
Sensors:  
          - OLED I2C
          - HDC100X I2C
          
       
          */

#include <SoftwareSerial.h>
//display
#include <SeeedGrayOLED.h>
#include <Wire.h>
//temperature and humidity
#include <HDC100X.h>
//SD-Card
#include <SD.h>
#include <SPI.h>
#include <RV8523.h>


RV8523 rtc;

uint8_t sec, min, hour, day, month;
uint16_t year;



SoftwareSerial mySerial(66, 67);

//I2C settings for the HDC100X
HDC100X HDC1(0x43);
//Settings for SD-Shield
#define serialSD 4


File file;
String nameOfFile;

boolean firstTime = true;

const int switchPin = 31;
int switchState = 0;

byte cmd[9] = {0xFF,0x01,0x86,0x00,0x00,0x00,0x00,0x00,0x79}; 
char response[9]; 
String ppmString = " ";

void setup()
{

  mySerial.begin(9600);
  Wire.begin();
  Serial.begin(9600);

  HDC1.begin(HDC100X_TEMP_HUMI,HDC100X_14BIT,HDC100X_14BIT,DISABLE);
  
   //display
    SeeedGrayOled.init();             //initialize SEEED OLED display
    SeeedGrayOled.clearDisplay();     //Clear Display.
    SeeedGrayOled.setNormalDisplay(); //Set Normal Display Mode
    SeeedGrayOled.setVerticalMode();  // Set to vertical mode for displaying text
 
 
 SD.begin(serialSD);
 
pinMode(switchPin, INPUT);

rtc.start();

  //When the power source is removed, the RTC will keep the time.
  rtc.batterySwitchOverOn(); //battery switch over on
  
}


void loop()
{
  //get time from RTC
  rtc.get(&sec, &min, &hour, &day, &month, &year);
  
    //Prepare filename and open file on SD
    char charFileName[nameOfFile.length()+1]; 
    nameOfFile.toCharArray(charFileName, sizeof(charFileName));
    file = SD.open(charFileName, FILE_WRITE);
    
  
  
mySerial.listen();
    mySerial.write(cmd,9);
  mySerial.readBytes(response, 9);
  int responseHigh = (int) response[2];
  int responseLow = (int) response[3];
  int ppm = (256*responseHigh)+responseLow;

  ppmString = String(ppm); //int to string
  Serial.print("PPM ");
  Serial.println(ppm);
 
  Serial.println();
  
 // float to string
        char tString [20];
        dtostrf(HDC1.getTemp(), 4, 2, tString);
        char hString [20];
        dtostrf(HDC1.getHumi(), 4, 2, hString);
        char co2string [20];
        dtostrf (ppm, 4,0,co2string);
        
      
  
  SeeedGrayOled.setGrayLevel(15); //Set Grayscale level. Any number between 0 - 15.
  SeeedGrayOled.setTextXY(1, 0); //set Cursor to ith line, 0th column
  SeeedGrayOled.putNumber(hour);
  SeeedGrayOled.setTextXY(1, 2);
  SeeedGrayOled.putString(":");
  SeeedGrayOled.setTextXY(1,3);
  SeeedGrayOled.putNumber(min);
  SeeedGrayOled.setTextXY(1,5);
  SeeedGrayOled.putString(":");
  SeeedGrayOled.setTextXY(1,6);
  SeeedGrayOled.putNumber(sec);
  SeeedGrayOled.setTextXY(2,0);
  SeeedGrayOled.putString(co2string);
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
  SeeedGrayOled.setTextXY(9,0);
  SeeedGrayOled.putString(charFileName);

 
  
  // build one csv output string
        String output = "";
        output += tString;
        output += ";";
        output += hString;
        output += ";";
        output += co2string;
        output += ";";
        output += hour;
        output += ":";
        output += min;
        output += ":";
        output += sec;
        
        

        // write the data to the output channels
        writeData(output);
        
  switchState = digitalRead(switchPin);
  delay(6000);
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
        file.println("Temperature; Humidity; CO2; Time"); //write the CSV Header to the file
        file.close();
        }
  }

  //// output the text

  // serial output
  Serial.println(txt);

  if (switchState == HIGH)
{
  // sd card output
  file.println(txt);
  file.flush();
  SeeedGrayOled.setTextXY(10,0);
  SeeedGrayOled.putString("Saving");
}
else {
 SeeedGrayOled.setTextXY(10,0);
  SeeedGrayOled.putString("      ");
}
}
