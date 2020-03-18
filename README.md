# Artnet-DMX-SD
Arnet to DMX stored on SD card, then Played back from SD card

Tested using MADRIX v3
Setup MADRIX to broadcast Artnet to IP range (When STORE_DMX starts up, it displays its IP)
Ensure ArtAdress is OFF, Send Full Frames is OFF, PreSync and PostSync is OFF

STORE_DMX needs to be configured with number of Universes used (start = 1, max = DEPENDS)
PLAY_DMX needs to be configured with number of leds, data_pin, and frame rate recorded at (setup in MADRIX).

NOTE: Use Arduino ESP8266 library v2.50 to program STORE_DMX

STORE_DMX will only work if connected to Arduino Serial Monitor - use the monitor to check IP address and SD card initialization.

Data will be stored on SD card like DATA0, DATA1, DATA2 etc.

Project modified from: https://github.com/tangophi/Artnet_DMX_SD_Card to work using FastLED library. Refer to this link for troubleshooting.

Currently PLAY_DMX is set up to loop animations for a long time.

NOTE: Printing out values to Serial while storing animations will cause animations to lag.

We found having the same number of channels in each universe helped too...
