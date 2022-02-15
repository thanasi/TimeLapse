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
#include <math.h>
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

// minimum interval between photos
#define MIN_PHOTO_INTERVAL_S 3

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

// duration of time lapse (changing photo_interval_s or max_photos will update this)
float time_lapse_dur_s = 0;

// timer for photos
Chrono photoTimer;

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
void pulseAF() {
  auto start_time = millis();
  digitalWrite(AF_PIN, HIGH);
  while (millis() - start_time < PULSE_DUR) {}
  digitalWrite(AF_PIN, LOW);
}

void pulseShutter() {
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
int showID(int argc = 0, char**argv = NULL)
{
  shell.println(F( "Running " _FILENAME_ " V" _VERSION_ ", Built " __DATE__));
  return EXIT_SUCCESS;
}

// print current info
int showInfo(int argc=1, char**argv = NULL)
{
  shell.print("* num photos: ");
  shell.print(max_photos);
  shell.print("\n* photo interval: ");
  shell.print(photo_interval_s);
  shell.print("s\n* time lapse duration: ");
  shell.print(time_lapse_dur_s);
  shell.print("s\n* AF enabled: ");
  shell.print(af_enabled ? "TRUE" : "FALSE");
  shell.print("\n* currently capturing: ");
  shell.println(capturing ? "TRUE" : "FALSE");
  
  return EXIT_SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////
// set number of photos to take (or 0 for unlimited)
int setPhotoCount(int argc, char **argv)
{
  if (argc == 2)
  {
    if (!capturing)
    {
      max_photos = atoi(argv[1]);
      time_lapse_dur_s = (float) max_photos * photo_interval_s;
      
      shell.print("* num photos: ");
      shell.println(max_photos);
      shell.print("* time lapse duration: ");
      shell.print(time_lapse_dur_s);
      shell.println("s");
      
    }
    else
    {
      shell.println(F("Cannot update photo count mid-capture"));

    }

    return EXIT_SUCCESS;

  }
  return badArgCount(argv[0]);
}


/////////////////////////////////////////////////////////////////////////////////
// set time interval between photos
int setPhotoInterval(int argc, char **argv)
{
  if (argc == 2)
  {
    if (!capturing) 
    {
      photo_interval_s = atof(argv[1]);
      time_lapse_dur_s = (float) max_photos * photo_interval_s;

      shell.print("* photo interval: ");
      shell.print(photo_interval_s);
      shell.print("s\n* time lapse duration: ");
      shell.print(time_lapse_dur_s);
      shell.println("s");
    }

    else
    {
      shell.println(F("Cannot update photo interval mid-capture"));

    }
    return EXIT_SUCCESS;
  }
  return badArgCount(argv[0]);
}

// set total number of photos based on a specified time lapse duration
int setTimeLapseDuration(int argc, char **argv)
{
  if (argc==2)
  {
    if (!capturing){
      time_lapse_dur_s = atof(argv[1]);
      max_photos = (int) floor(time_lapse_dur_s / photo_interval_s);
      
      shell.print("* time lapse duration: ");
      shell.print(time_lapse_dur_s);
      shell.print("s\n* num photos: ");
      shell.println(max_photos);
    }
    else
    {
      shell.println(F("Cannot update time lapse duration mid-capture"));
    }
    return EXIT_SUCCESS;
  }
  return badArgCount(argv[0]);
}

/////////////////////////////////////////////////////////////////////////////////
// trigger single photo
int triggerSinglePhoto(int argc = 0, char** argv = NULL)
{

  if (af_enabled) { pulseAF(); }

  pulseShutter();

  return EXIT_SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////
// stop time lapse
int stopTimeLapse(int argc = 1, char **argv = NULL)
{
  capturing = false;
  return EXIT_SUCCESS;
}

// increment capture count
void incrCapCount()
{
  num_photos += 1;
  if (num_photos == max_photos)
  {
    stopTimeLapse(0, NULL);
  }
}

// start time lapse
int startTimeLapse(int argc = 1, char **argv = NULL)
{
  capturing = true;
  num_photos = 0;
  photoTimer.restart();
  triggerSinglePhoto(0, NULL);
  incrCapCount();
  return EXIT_SUCCESS;
}


/////////////////////////////////////////////////////////////////////////////////

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
  shell.addCommand(F("status?"), showInfo);
  shell.addCommand(F("trigger"), triggerSinglePhoto);
  shell.addCommand(F("start"), startTimeLapse);
  shell.addCommand(F("cancel"), stopTimeLapse);
  shell.addCommand(F("set_count"), setPhotoCount);
  shell.addCommand(F("set_dur"), setTimeLapseDuration);
  shell.addCommand(F("set_interval"), setPhotoInterval);


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

  if (capturing) {

    blinkLED_nb();

    if (photoTimer.hasPassed(photo_interval_s * 1000))
    {
      photoTimer.restart();
      triggerSinglePhoto(0, NULL);
      incrCapCount();

    }

  }

  else {
    // make sure LED isn't stuck on
    digitalWrite(LED_BUILTIN, LOW);

  }

}
