#include <SD.h>
#include <SoftwareSerial.h>
#include <SPI.h>
#include <TinyGPS++.h>


//Place for definitions//
#define redLED 3
#define greenLED 4
#define RX 7
#define TX 6
#define chipSelect 10 //not really sure what this does right now


TinyGPSPlus GPS;
SoftwareSerial gps_serial(7,6);


File datalog;                     //File object for datalogging
char filename[] = "COPGPS00.csv"; //Template for file name to save data
bool SDactive = false;            //used to check for SD card before attempting to log


//functions to manage the GPS in updateGPS().
unsigned long lastGPS = 0;
unsigned long GPSstartTime = 0;    //when the GPS starts, time in seconds of last GPS update.
uint8_t days = 0;                  //if we're flying overnight
unsigned long timer = 0;           //timer to ensure GPS prints data once every second


void updateGPS() {                    //Function that updates GPS every second and accounts for
  static bool firstFix = false;       //clock rollover at midnight (23:59:59 to 00:00:00)
  while (gps_serial.available() > 0) {
    GPS.encode(gps_serial.read());
    }
  if (GPS.altitude.isUpdated() || GPS.location.isUpdated()) {
    if (!firstFix && GPS.Fix) {     //gps.fix
      GPSstartTime = GPS.time.hour() * 3600 + GPS.time.minute() * 60 + GPS.time.second();
      firstFix = true;
      }
    }
  if (getGPStime() > lastGPS) {    //if it's been more than a second
        lastGPS = GPS.time.hour() * 3600 + GPS.time.minute() * 60 + GPS.time.second();
    }
}
   
int getGPStime() {
  return (GPS.time.hour() * 3600 + GPS.time.minute() * 60 + GPS.time.second());
}

int getLastGPS() { 
  //returns time in seconds between last successful fix and initial fix. Used to match with altitude data
  static bool newDay  = false;           //variable in case we're flying late at night (clock rollover)
  if (!newDay && lastGPS < GPSstartTime) {
    days++;
    newDay = true;
  }
  else if (newDay && lastGPS > GPSstartTime)
    newDay = false;
  return days * 86400 + lastGPS;
}

String printGPS() { //function that returns the GPS data
  //populate the variables in case the GPS does not get a fix
  String datestring = "01/01/2000";
  String timestamp = "00:00:00";
  String coordinates = "00.0000, 00.0000,  Altitude(feet): 0.000";
  
  if (GPS.Fix && GPS.altitude.feet() != 0) { //populate the strings with the GPS data
    datestring = String(GPS.date.month()) + "/" + String(GPS.date.day()) + "/" + String(GPS.date.year());
    timestamp = String(GPS.time.hour()) + ":" + String(GPS.time.minute()) + ":" + String(GPS.time.second());
    coordinates = String(GPS.location.lat(), 6) + ", " + String(GPS.location.lng(), 6) + ",  Altitude (feet): " + String(GPS.altitude.feet());
    }
  
  String data = datestring + ", " + timestamp + ",  " + coordinates; //date,time,lat,long,alt
  return data;
}





void setup() {
  Serial.begin(9600);
  pinMode(redLED, OUTPUT);
  pinMode(greenLED, OUTPUT);
  pinMode(10, OUTPUT);    //This always needs to be an output when using SD

  gps_serial.begin(4800);

  Serial.print("Initializing SD card...");
  if(!SD.begin(chipSelect)) {                               //attempt to start SD communication
    Serial.println("Card failed, or not present");          //print out error if failed; remind user to check card
    for (byte i = 0; i < 10; i++) {                         //also flash error LED rapidly for 2 seconds, then leave it on
      digitalWrite(redLED, HIGH);
      delay(100);
      digitalWrite(redLED, LOW);
      delay(100);
    }
    digitalWrite(redLED, HIGH);
  }
  else {                                                    //if successful, attempt to create file
    Serial.println("Card initialized.\nCreating File...");
    for (byte i = 0; i < 100; i++) {                        //can create up to 100 files with similar names, but numbered differently
      filename[6] = '0' + i/10;
      filename[7] = '0' + i%10;
      if (!SD.exists(filename)) {                           //if a given filename doesn't exist, it's available
        datalog = SD.open(filename, FILE_WRITE);            //create file with that name
        SDactive = true;                                    //activate SD logging since file creation was successful
        Serial.println("Logging to: " + String(filename));  //Tell user which file contains the data for this run of the program
        break;                                              //Exit the for loop now that we have a file
      }
    }
    if (!SDactive) {
      Serial.println("No available file names; clear SD card to enable logging");
      for (byte i = 0; i < 4; i++) {                        //flash LED more slowly if error is too many files (unlikely to happen)
        digitalWrite(redLED, HIGH);
        delay(250);
        digitalWrite(redLED, LOW);
        delay(250);
      }
      digitalWrite(redLED, HIGH);
    }
  }

  String header = "GPS Date, GPS time, Latitude, Longitude, Altitude(ft)";  //setup data format, and print it to monitor and SD card
  Serial.println(header);
  if (SDactive) {
    datalog.println(header);
    datalog.close();
  }

}



void loop() {
  updateGPS(); //time-keeping stuff for the GPS

  if (millis() - timer > 1000) {
    timer = millis();

    digitalWrite(greenLED, HIGH);
    Serial.println("hi");
    Serial.println(GPS.altitude.feet());
    Serial.println(printGPS()); //Prints GPS data to serial monitor
    if (SDactive) {
        datalog = SD.open(filename, FILE_WRITE);
        datalog.println(printGPS());
        datalog.close();
        }
     digitalWrite(greenLED, LOW);
    }

}
