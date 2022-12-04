 /*
 * Connect the SD card to the following pins:
 *
 * SD Card | ESP32
 *    D2       -
 *    D3       SS
 *    CMD      MOSI
 *    VSS      GND
 *    VDD      3.3V
 *    CLK      SCK
 *    VSS      GND
 *    D0       MISO
 *    D1       -
 */
#include "FS.h"
#include "SD.h"
#include "SPI.h"

#include "HX711.h"

////////////////////////////////////////////////////////////////////globals

// Data Arrays for Time and Force
double forceVals[100];
double timeVals[100];

// Defining pins for Load Cell
#define DT  26
#define SCK  25

const int IGN = 33; // Igniter pin

// Counter variables
int i = -1;
int counter = 0;

// Arrays for action codes
char Data[2];
char Data2[2];
char Data3[1];

// Action Phase booleans
bool STANDBY = true;
bool ARMED = false;
bool PRE_IGNITION = false;
bool DATA_ACQUISITION = false;
bool IGNITED = false;
bool DATA_DUMP = false;

// Action codes used for advancing through different phases
char ArmCd[4] = "!AR";
char IgnCd[4] = "!IG";
char AbCd[2] = "!";


// Creating Scale object
HX711 scale;
float calibration_factor = 1100; // Outputs Newtons, calibration for 200kg = 1270, calibration for 50 kg = 
float Tstart = millis(); // Value used for time as a reference
////////////////////////////////////////////////////////////////////writeFile and appendFile method

// Writing to and creating a file
void writeFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Writing file: %s\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("File written");
    } else {
        Serial.println("Write failed");
    }
    file.close();
}

// Adding on to a file that is already made
void appendFile(fs::FS &fs, const char * path, String message){
    Serial.printf("Appending to file: %s\n", path);

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("Failed to open file for appending");
        return;
    }
    if(file.print(message)){
        Serial.println("Message appended");
    } else {
        Serial.println("Append failed");
    }
    file.close();
}

/////////////////////////////////////////////////////////////////////

void setup()
{
  Serial.begin(9600); // Creating Serial

  // Setting up Igniter Relay
  pinMode(IGN, OUTPUT);
  digitalWrite(IGN, LOW);
  
  // Load Cell calibration and initialization 
  Serial.println("HX711 calibration");
  Serial.println("Remove all weight from scale");
  scale.begin(DT, SCK);
  scale.set_scale(calibration_factor);
  scale.tare();
  int zero_factor = scale.read_average();
  Serial.print("Zero factor: ");
  Serial.println(zero_factor);

  // SD Card Reader initialization test
  Serial.print("Initializing SD card...");
  if(!SD.begin())
  {
    Serial.println("Card Mount Failed");
    DATA_ACQUISITION = false;
    return;
  }
  
  Serial.println("initialization done.");
  writeFile(SD, "/C6_Static_Fire_2_4_17_2021.txt","Time - Force \n"); // Writes a header and makes a new text file to write to
  delay(5000);
}

void loop()
{
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////    
// STANDBY PHASE
  while (STANDBY)
  {
    Serial.println("Standby Phase Active");
    counter = 0;

    // Checks serial monitor for input
    while(Serial.available() > 0)
    {
      char databit = Serial.read(); 
      Data[counter] = databit;      
      counter++;                    
    }

    // Checks if input is same as action code
    if (Data[0] == ArmCd[0] && Data[1] == ArmCd[1] && Data[2] == ArmCd[2])
    {
      // Moves into ARMED PHASE
      STANDBY = false;
      ARMED = true;
    } 
  }
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // ARMED PHASE
  while (ARMED)
  {
    Serial.println("Armed Phase Active");
    counter = 0;

    // Checks serial monitor for input
    while(Serial.available() > 0)
    {
      char databit = Serial.read();
      Data2[counter] = databit;
      counter++;
    }

    // Checks if input is same as action code
    if (Data2[0] == IgnCd[0] && Data2[1] == IgnCd[1] && Data2[2] == IgnCd[2])
    {
      // Moves into PRE-IGNITION PHASE
      ARMED = false;
      PRE_IGNITION = true;
    } 
  }
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // PRE-IGNITION PHASE 
  while (PRE_IGNITION)
  {
    Serial.println("Pre-Ignition Phase Active");
    counter = 0;

    // Checks serial monitor for input
    while(Serial.available() > 0)
    {
      char databit = Serial.read();
      Data3[counter] = databit;
      counter++;
    }
    
    // Checks if input is same as action code
    if (Data3[0] == AbCd[0])
    {
      // Moves into DATA_ACQUISITION PHASE
      PRE_IGNITION = false;
      DATA_ACQUISITION = true;
    } 

    // Creates values to be used in Data Acquisition Phase
    Tstart = millis();
    scale.set_scale(calibration_factor); // Scale set with calibration factor made above
  }
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////  
  // DATA ACQUISITION PHASE
  Serial.println("Data Acquisiton Phase Active!");
  while(DATA_ACQUISITION)
  {
    double scaleReading = scale.get_units(); // Reading taken from load cell stored here
    float t = (millis() - Tstart) / 1000; // Time value used for data reading

    // Igniter Loop
    // After 1 second ignite
    if ((t >= 1) && (IGNITED == false))
    {
      digitalWrite(IGN, HIGH); // Ignites E-Match
      Serial.println("IGNITION TIME");
      IGNITED = true; // When true then ignition can't be reignited until rewritten to false
    }

    // Effectively a for-loop
    // Used to write load cell reading and time reading into data array
    i++;
    forceVals[i] = scaleReading;
    timeVals[i] = t;
    Serial.print(forceVals[i]);
    Serial.print(", ");
    Serial.println(timeVals[i]);
    
    // After 100 data points are written then jump into new action phase
    // Moves into DATA_DUMP PHASE
    if (i == 99){
      digitalWrite(IGN, LOW);
      Serial.println("done");
      DATA_ACQUISITION = false;
      DATA_DUMP = true;      
    }
  }
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////  
  // DATA_DUMP PHASE 
  while(DATA_DUMP)
  {
    Serial.println("Data Dump Phase Active");
    
    // Loop for writing into SD Card
    for (int j = 0; j < 100; j++)
    {
      Serial.println("DUMPING DATA!");
      appendFile(SD, "/test2.txt", String(forceVals[j]));
      appendFile(SD, "/test2.txt", ", ");
      appendFile(SD, "/test2.txt", String(timeVals[j]));
      appendFile(SD, "/test2.txt", " \n");
    }
    Serial.println("DATA DUMP SUCCESSFULL!"); // Checkpoint to ensure data writing successful
    
    // Move into STANDBY PHASE
    DATA_DUMP = false; // Exits DATA_DUMP
    IGNITED = false; // Rewrites ignited to false to let it re-ignite without resetting the board fully
    STANDBY = true; // Enters STANDBY PHASE again
    Serial.println("GOING BACK TO STANDBY PHASE");
    delay(15000); // Delay to give user time to work on board or anything like that
  }
}
