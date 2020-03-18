  /*
 * This sketch is used to read the stored LED lighting effects from a SD card on a 
 * nodemcu or a ESP8266 module.  'Next' button will play the file or effect.  Previous
 * button will play the previous effect.
 * 
 * WiFi connection is not needed for this sketch to work.
 * 
 * Update FRAME_TIME to match your setup.
 *  
 * https://github.com/tangophi/Artnet_DMX_SD_Card
*/

#include <SPI.h>
#include <SD.h>
#include <FastLED.h>


/***********************************************************
/* These settings need to be changed according to your setup
/************************************************************/

#define FRAME_TIME            30  // CHANGE FOR YOUR SETUP.  Should be the same as the Frame Time in Madrix -> Preferences-> Device Manager-> DMX Devices config.

#define PIN_PREVIOUS_BUTTON   4
#define PIN_NEXT_BUTTON       5
#define PIN_SD_CS             BUILTIN_SDCARD
#define PIN_LED               3

// Neopixel settings
const int numLeds = 300;          // CHANGE FOR YOUR SETUP.

/************************************************************/

CRGB leds[numLeds];

int fail = 0;

const int numberOfChannels = numLeds * 3; // Total number of channels you want to receive (1 led = 3 channels)
byte channelBuffer[numberOfChannels];     // Combined universes into a single array

// SD card
File datafile;

char fileNameFull[10] = "";
int  fileNameSuffix   = 0;
int  totalFilesCount  = 0;

volatile bool prevFile = false;
volatile bool nextFile = false;

void initTest()
{
  for (int i = 0 ; i < numLeds-1 ; i++) {
    leds[i] = CRGB(127, 0, 0);
  }
  FastLED.show();
  delay(500);
  for (int i = 0 ; i < numLeds-1 ; i++) {
    leds[i] = CRGB(0, 127, 0);
  }
  FastLED.show();
  delay(500);
  for (int i = 0 ; i < numLeds-1 ; i++) {
    leds[i] = CRGB(0, 0, 127);
  }
  FastLED.show();
  delay(500);
  for (int i = 0 ; i < numLeds-1 ; i++) {
    leds[i] = CRGB(0, 0, 0);
  }
  FastLED.show();
}

void buttonHandlerPrevious()
{
  if (!prevFile)
  {
    prevFile = true;
    Serial.println("Previous button pressed.");
  }
}

void buttonHandlerNext()
{
  if (!nextFile)
  {
    nextFile = true;
    Serial.println("Next button pressed.");
  }
}

void setup()
{
  Serial.begin(115200);
  FastLED.addLeds<WS2812B, PIN_LED, GRB>(leds, numLeds);
  initTest();
  
  while (fail == 0){
    if (!SD.begin(PIN_SD_CS)) {
      Serial.println("SD card initialization failed!");
        int hue = 0;
  for (int k=0; k<numLeds-1; k++){
    if (hue < 255 || hue == 255){
      fill_solid(leds, numLeds-1, CHSV(hue,255,255));
      hue++;
      FastLED.show();
      delay(10);
    }
    else{
      hue=0;
 
    }
  for (int k=0; k<numLeds-1; k++){
    if (hue >0 || hue == 0){
      fill_solid(leds, numLeds-1, CHSV(hue,255,255));
      hue--;
      FastLED.show();
      delay(10);
    }
    else{
      hue=255;
 
    }
  }
  }
    }
    else
    {
      Serial.println("SD card initialization done.");
      fail = 1;
    }
  }
  


  attachInterrupt (PIN_PREVIOUS_BUTTON,  buttonHandlerPrevious,  RISING);
  attachInterrupt (PIN_NEXT_BUTTON,      buttonHandlerNext,      RISING);

  // Get the total number of 'dataXXX' files on the SD card.
  for (int i=0;i<10000;i++)
  {
    sprintf(fileNameFull, "data%d", i);
    if(SD.exists(fileNameFull))
    {
      totalFilesCount = i + 1;
    }
    else
    {
      break;
    }
  }

  Serial.print("Total number of data files on the SD card: ");
  Serial.println(totalFilesCount);

  memset(fileNameFull, 0, 10);
  sprintf(fileNameFull, "data%d", fileNameSuffix);
  datafile = SD.open(fileNameFull, FILE_READ);
}

void loop()
{
  // Close the current file and open the previous file
//  if (prevFile)
//  {
//    datafile.close();
//
//    if (--fileNameSuffix < 0)
//    {
//      fileNameSuffix = totalFilesCount - 1;
//    }
//
//    memset(fileNameFull, 0, 10);
//    sprintf(fileNameFull, "data%d", fileNameSuffix);
//
//    Serial.print("Opening ");
//    Serial.println(fileNameFull);
//    
//    datafile = SD.open(fileNameFull, FILE_READ);
//
//    delay(100);
//    prevFile = false;
//  }

  // Close the curent file and open the next file
  if (nextFile)
  {
    datafile.close();

    if (++fileNameSuffix >= totalFilesCount)
    {
      fileNameSuffix = 0;
    }

    memset(fileNameFull, 0, 10);
    sprintf(fileNameFull, "data%d", fileNameSuffix);

    Serial.print("Opening ");
    Serial.println(fileNameFull);
    
    datafile = SD.open(fileNameFull, FILE_READ);

    delay(100);
    nextFile = false;
  }

  // Keep reading data from the file and setting the LEDs.  If the full file has been read, then reset
  // the file pointer to the beginning of the file.
  if (datafile.available())
  {
      for (int t=0;t<50000;t++){
         if (datafile.available())
          {   
          //Serial.println("data available here!");
          datafile.read(channelBuffer, numberOfChannels);
          for (int i = 0; i < numLeds-1; i++){
            leds[i]=CRGB(channelBuffer[(i) * 3], channelBuffer[(i * 3) + 1], channelBuffer[(i * 3) + 2]);
            //Serial.println(channelBuffer[(i) * 3]);
          }
          FastLED.show();
          delay(FRAME_TIME);
    }
         else
            {
           //Serial.println("No data available...");
             datafile.seek(0);
            }
  }
  }
  
  else
  {
    //Serial.println("No data available...");
    datafile.seek(0);
  }
  nextFile=true;
  delay(10);
}
