/*
 *  This sketch is used to store DMX frames received through ArtNet protocol to a SD card on a nodemcu
 *  or a ESP8266 module.  'Start' button is to start recording frames while 'Stop' button is to stop 
 *  recording.  Multiple effects can be stored in individual files with incremental names.
 *  
 *  LED strip need not be connected for this sketch to work.  However the strip need to be defined
 *  in the sketch so that the sketch can calculate the number of pixels, channels and universes.
 *  
 *  Requirements:
 *  -------------
 *  All the universes should be sequential.  That is if first universe is 10, second should be 11, 
 *  third should be 12, etc...
 *  
 *  Madrix should be configured to send the first universe first, second next, and so on.
 *  
 *  In Madrix, in Preferences-> Device Manager-> DMX Devices, make sure "Send full frames" box is 
 *  not checked.
 *  
 * https://github.com/tangophi/Artnet_DMX_SD_Card 
*/

#include <SPI.h>
#include <SD.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ArtnetWifi.h>

/***********************************************************
/* These settings need to be changed according to your setup
/************************************************************/

#define PIN_START_BUTTON   4
#define PIN_STOP_BUTTON    5
#define PIN_SD_CS          15
#define PIN_LED            3

int fail = 0;

//Wifi settings
const char* ssid = "ThingkingStudio(Pty)Ltd Sonic";         // CHANGE FOR YOUR SETUP
const char* password = "Kelvin!273"; // CHANGE FOR YOUR SETUP

// LED settings
const int numLeds = 300; // CHANGE FOR YOUR SETUP
//const int numberOfChannels = numLeds * 3; // Total number of channels you want to receive (1 led = 3 channels)
//const byte dataPin = 3;
//CRGB leds[numLeds];

// Madrix settings
const int firstUniverse =  1;      // CHANGE FOR YOUR SETUP
const int lastUniverse   = 2;      // CHANGE FOR YOUR SETUP     

/************************************************************/

// Artnet settings
ArtnetWifi artnet;
const int numberOfChannels = numLeds * 3; // Total number of channels you want to receive (1 led = 3 channels)
byte channelBuffer[numberOfChannels];     // Combined data from all universes into a single array

// SD card
File datafile;

char fileNameFull[10] = "";
int  fileNameSuffix   = 0;

volatile bool startRecord = false;
volatile bool stopRecord  = false;
volatile bool recording   = false;

int bufferIndex = 0;

// Check if we got all universes
const int maxUniverses = lastUniverse - firstUniverse + 1;
bool universesReceived[maxUniverses];
bool storeFrame = 1;

// connect to wifi – returns true if successful or false if not
boolean ConnectWifi(void)
{
  boolean state = true;
  int i = 0;

  WiFi.begin(ssid, password);
  Serial.println("");
  Serial.println("Connecting to WiFi");
  
  // Wait for connection
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (i > 20){
      state = false;
      break;
    }
    i++;
  }
  if (state){
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("");
    Serial.println("Connection failed.");
  }
  
  return state;
}


void buttonHandlerStart()
{
  if (!recording && !startRecord)
  {
    startRecord = true;
    Serial.println("Start button pressed.");
  }
}

void buttonHandlerStop()
{
  if (recording && !stopRecord)
  {
    stopRecord = true;
    Serial.println("Stop button pressed.");
  }
}

void setup()
{
  Serial.begin(115200);

  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  
  while (fail == 0){
    if (!SD.begin(PIN_SD_CS)) {
      Serial.println("SD card initialization failed!");
    }
    else
    {
      Serial.println("SD card initialization done.");
      fail = 1;
    }
  }
  
  ConnectWifi();
  artnet.begin();
  
  attachInterrupt (PIN_START_BUTTON,   buttonHandlerStart,   RISING);
  attachInterrupt (PIN_STOP_BUTTON,    buttonHandlerStop,    RISING);

  sprintf(fileNameFull, "data%d", fileNameSuffix);
  
  // this will be called for each packet received
  artnet.setArtDmxCallback(onDmxFrame);

  Serial.printf("Initial config: numLeds=%4d : numberOfChannels=%4d : firstUniverse=%3d : lastUniverse=%3d\n", numLeds, numberOfChannels, firstUniverse, lastUniverse);
}

void loop()
{
  char c;
  // we call the read function inside the loop
  artnet.read();

  if (Serial.available())
  {
    c = Serial.read();

    if (c == 's')
    {
      if (!recording && !startRecord)
      {
        startRecord = true;
        Serial.println("Start button pressed.");
      }    
    }
    else if (c == 'e')
    {
      if (recording && !stopRecord)
      {
        stopRecord = true;
        Serial.println("Stop button pressed.");
      }
    }
  }
  
  // Open a file for writing when the start button is pressed
  // and also set recording to true so that incoming DMX frames
  // are written to the file.
  if (startRecord && !recording)
  {
    datafile = SD.open(fileNameFull, FILE_WRITE);
    
    Serial.print("Opening ");
    Serial.println(fileNameFull);

    startRecord = false;
    recording = true;
  }

  // Stop the recording when the stop button is pressed and close 
  // the current file which was earlier opened for writing.  Also 
  // increment the fileNameFull variable.
  if (recording && stopRecord)
  {
    recording = false;
    delay(30);
    Serial.print("Closing ");
    Serial.println(fileNameFull);

    datafile.close();
    sprintf(fileNameFull, "data%d", ++fileNameSuffix);  // fileNameSuffix is incremented for the next filename.
    stopRecord = false;
  }
}

// This function is called for every packet received.  It will contain data for only
// one universe.  Hence, wait till data for all universes are received before 
// writing a full frame to the file.
void onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data)
{
  storeFrame = 1;

  // The universe Madrix sends become decremented by 1 when it reaches here.  Not sure why.
  // But increment it back up.
  universe++;

  //Serial.printf("DMX recd: univ=%3d : len=%3d\n", universe, length);
  
  // Store which universe has got in now
  if ((universe >= firstUniverse) && (universe <= lastUniverse))
  {
    // On the start of a full frame (all universes constitute a full frame), wait 
    // till the firstUniverse is received.  Otherwise exit.
    if ((bufferIndex == 0) && (universe != firstUniverse))
    {
      return;
    }
    universesReceived[universe - firstUniverse] = 1;
  }
  
  // See if data for all universes is received.  If it is, then storeFrame will still be 1 and in the next
  // code block, the full DMX frame (containing data for all the universes) will be written to the file.
  for (int i = 0 ; i < maxUniverses ; i++)
  {
    if (universesReceived[i] == 0)
    {
      //Serial.println("Break");
      storeFrame = 0;
      break;
    }
  }

  
  
  // Read universe data and put into the right part of the display buffer
  for (int i = 0 ; i < length ; i++)
  {
    
    if (bufferIndex < numberOfChannels)
    {
      //Serial.printf("bufI=%3d : i=%3d : d=%d\n", bufferIndex, i, data[i]); 
      channelBuffer[bufferIndex++] = byte(data[i]);
    }
      
  }
  
  // Write data to the file after DMX frames containing data for all the universes 
  // is received and if we are still recording
  if (recording && storeFrame)
  { 
    //Serial.printf("Into file: numChannels=%3d : maxUnivs=%d : bufIdx=%3d \n", numberOfChannels, maxUniverses, bufferIndex-1);    
    datafile.write(channelBuffer, numberOfChannels);
    memset(universesReceived, 0, maxUniverses);
    bufferIndex = 0;
  } 
}
