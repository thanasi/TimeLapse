/*********************************************************
arduino code to trigger time lapse images
by pulsing on digital IO

Version: 1.0
Date:    February 2022
Author:  A G Athanassiadis
*********************************************************/
# define _VERSION_ "1.0"
# define _FILENAME_ "TimeLapse.ino"

#include <Arduino.h>
#include <SimpleSerialShell.h>
#include <Chrono.h>


/*********************
 Static Definitions
*********************/

// internal LED
#define LED_PIN 13

// pin to trigger shutter
#define SHUTTER_PIN 2

// pin to trigger autofocus
#define AF_PIN 3

// pulse duration (milliseconds)
#define PULSE_DUR 100


/*********************
 Variables
*********************/

// current capture state
bool capturing = false;

// number of photos to take
int max_photos = 1;

// number of photos taken so far
int num_photos = 0;

// autofocus before taking image?
bool af_enabled = false;

// timing between photos (seconds)
float photo_interval_s = 1;

// timer for photos
Chrono photoTimer = Chrono();

/**********************
 Setup
***********************/
void setup() {

	pinMode(LED_PIN, OUTPUT);
	pinMode(SHUTTER_PIN, OUTPUT);
    pinMode(AF_PIN, OUTPUT);

	digitalWrite(LED_PIN, LOW);
	digitalWrite(SHUTTER_PIN, LOW);
    digitalWrite(AF_PIN, LOW);
  

	Serial.begin(115200);
	while (!Serial) {} // wait for serial to connect

	Serial.write(0xC); // write Form Feed character to clear the serial terminal (when supported)
	shell.attach(Serial);

	shell.addCommand(F("file?"), showID);
    shell.addCommand(F("trigger"), triggerSinglePhoto);
	shell.addCommand(F("start"), startTimeLapse);
    shell.addCommand(F("cancel"), stopTimeLapse);
	shell.addCommand(F("set_count"), setPhotoCount);
    shell.addCommand(F("set_dur"), setPhotoDuration);
    shell.addCommand(F("set_spacing"), setPhotoSpacing);


	shell.println();
	showID();
	shell.println();
	shell.execute("help");
	shell.println();
}


/**********************
 Main Loop  
***********************/
void loop() {
  
  shell.executeIfInput();
  
  if(capturing){
    blinkLED_nb();
    // run capture routine, using Chrono tracker to track timing between shots
     if photoTimer.metro(photo_interval_s*1000)
     {
      triggerSinglePhoto(0,NULL);
      incrCapCount();
     }

	  
	  
  }
  
}



/*********************
 Function Definitions
*********************/

/////////////////////////////////////////////////////////////////////////////////
// non blocking LED toggle
void blinkLED_nb(void)
{
  static auto lastToggle = millis();  // saved between calls
  auto now = millis();

  if (now - lastToggle > 1000)
  {
    // toggle
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    lastToggle = now;
  }
}

/////////////////////////////////////////////////////////////////////////////////
// blocking pulse for AF and shutter
void pulseAF(){
	auto start_time = millis();
	digitalWrite(AF_PIN, HIGH);
	while (millis() - start_time < PULSE_DUR) {}
	digitalWrite(AF_PIN, LOW);
}

void pulseShutter(){
	auto start_time = millis();
	digitalWrite(SHUTTER_PIN, HIGH);
	while (millis() - start_time < PULSE_DUR) {}
	digitalWrite(SHUTTER_PIN, LOW);
}


/////////////////////////////////////////////////////////////////////////////////
// helper function to raise error if incorrect number of arguments specified
int badArgCount( char * cmdName )
{
	shell.print(cmdName);
	shell.println(F(": bad arg count"));
	return EXIT_FAILURE;
}


/////////////////////////////////////////////////////////////////////////////////
// helper function to print current file version
int showID(int argc=0, char**argv=NULL)
{
	shell.println(F( "Running " _FILENAME_ " V" _VERSION_ ", Built " __DATE__));
}


/////////////////////////////////////////////////////////////////////////////////
// set number of photos to take (or 0 for unlimited)
int setPhotoCount(int argc, char **argv)
{
  if (argc==2)
  {
	  if (!capturing)
	  {
		  max_photos = atoi(argv[1]);
	  }
	  else
	  {
		shell.println(F( "Cannot update photo count mid-capture"));

	  }
	  
	  return EXIT_SUCCESS;
	  
  }
  return badArgCount(argv[0]);
}


/////////////////////////////////////////////////////////////////////////////////
// set time interval to take photos for (by setting total number of photos)
int setPhotoInterval(int argc, char **argv)
{
  if (argc==2)
  {
	  if (!capturing){
	  photo_interval_s = atof(argv[1]);
	  }
	  else
	  {
		shell.println(F( "Cannot update photo interval mid-capture"));
		
	  }
	  return EXIT_SUCCESS;
  }
  return badArgCount(argv[0]);
}

/////////////////////////////////////////////////////////////////////////////////
// trigger single photo
int triggerSinglePhoto(int argc=0, char** argv=NULL)
{

  if (af_enabled) { pulseAF(); }
	
  pulseShutter();
 
  return EXIT_SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////
// increment capture count
void incrCapCount()
{
	num_photos += 1;
	if (num_photos==max_photos) 
	{
		stopTimeLapse(0,NULL);
	}
}


/////////////////////////////////////////////////////////////////////////////////
// start time lapse
int startTimeLapse(int argc=1, char **argv=NULL)
{
  capturing = true;
  num_cap = 0;
  photoTimer.restart();
  triggerSinglePhoto(0,NULL);
  incrCapCount();
  return EXIT_SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////
// stop time lapse
int stopTimeLapse(int argc=1, char **argv=NULL)
{
  capturing = false;
  return EXIT_SUCCESS;
}



/////////////////////////////////////////////////////////////////////////////////

