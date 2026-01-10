#include <WiFi.h>
#include <esp_now.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>  // Library ADS1115

#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <EEPROM.h>

#include <U8g2lib.h>

bool settingMode;
bool oledMode = 0;
unsigned long periodMs;
float frequencyHz = 0.0;

//================= ADS1115 Configuration =================//
Adafruit_ADS1115 ads;

// ADS1115 Calibration (sesuaikan dengan joystick kamu)
int joyCenter[4] = {13055, 13119, 12815, 13165};
int joyMin[4]    = {11, 10, 11, 9};
int joyMax[4]    = {26319, 26139, 26326, 26325};
int deadzone = 150;

//================= PCF8575 Configuration =================//
uint8_t PCF8575_ADDR = 0xFF;
uint16_t pcfData = 0xFFFF;

/*
  PCF8575 Button Mapping (16 buttons):
  Bit 0  -> UP
  Bit 1  -> RIGHT
  Bit 2  -> DOWN
  Bit 3  -> LEFT
  Bit 4  -> SQUARE
  Bit 5  -> CROSS
  Bit 6  -> CIRCLE
  Bit 7  -> TRIANGLE
  Bit 8  -> R1
  Bit 9  -> R2
  Bit 10 -> L3
  Bit 11 -> R3
  Bit 12 -> LED_BTN (not used in stat array)
  Bit 13 -> L1
  Bit 14 -> L2
  Bit 15 -> SWITCH (button[14])
*/

//============================EDIT THIS TO CHANGE THE RX MAC ADDRESS==================================//
uint8_t broadcastAddress[6][6] = {
  {0x0C, 0xB8, 0x15, 0xC2, 0x9A, 0x78}, // ESPnya alfi
  //{0x80, 0x7D, 0x3A, 0xEA, 0xB1, 0x98}, // esp32 ext ant R1
  {0x8C, 0x4F, 0x00, 0x3C, 0x91, 0x7C}, // esp32 v1 microUSB 
  {0x14, 0x33, 0x5C, 0x02, 0x34, 0x58}, // esp32 ext antenna 2
  {0x14, 0x33, 0x5C, 0x02, 0x49, 0x58}, // esp32 ext antenna 3 
  {0xF0, 0x08, 0xD1, 0xC8, 0x52, 0xEC}, // esp32 baru 1
  {0x0C, 0xB8, 0x15, 0xC2, 0x9A, 0x78} // ESPnya alfi
};
int macIndex;

//=========================VARS TO store incoming data - MUST MATCH WITH TX================================//
typedef struct receivedData {
  int rpmData[2];
  float voltData[3];
  int pressureData;
} receivedData;
receivedData incomingData;

//=========================VARS TO SENT BY ESPNOW - MUST MATCH WITH RX================================//
typedef struct struct_message {
  bool stat[16];
  int joyData[4];
  uint8_t remoteIndex;
} struct_message;

struct_message sendData;

// //==============================VARS FOR BUTTON (now via PCF8575, keep for OLED compatibility)=========//
// // These are virtual button indices for display purposes only
// // Actual button reading is done via PCF8575 in B_ESPNOW_Functions
// uint8_t button[15] = {33, 18, 16, 17, 4, 15, 13, 12, 25, 26, 27, 14, 19, 23, 32}; 
//  // "L3", "R3", "L2", "L1", "UP", "LEFT", "DOWN", "RIGHT", "SQUARE", "CROSS", "ROUND", "TRIANGLE", "R2", "R1", SW
//  //   0    1     2     3     4       5       6       7         8         9       10        11       12    13   14
// //==============================VARS FOR JOYSTICK (now via ADS1115)=====================================//
// // Keep these for OLED display position calculation
// int joy1XPos, joy1YPos, joy2XPos, joy2YPos;
// const int boxSize = 30;
// const int joy1XBox = 48;
// const int joy1YBox = 47;
// const int joy2XBox = 80;
// const int joy2YBox = 47;

//==============================VARS FOR JOYSTICK (now via ADS1115)=====================================//
// Keep these for OLED display position calculation
int joy1XPos, joy1YPos, joy2XPos, joy2YPos;  // ✅ HARUS AKTIF untuk OLED!
const int boxSize = 30;
const int joy1XBox = 48;
const int joy1YBox = 47;
const int joy2XBox = 80;
const int joy2YBox = 47;

//==============================VARS FOR BATT VOLTAGE MEASUREMENT=======================================//
uint8_t voltPin = 32; 
float batteryVoltage, batteryPercentage;
const float voltageDividerFactor = 47.0 / (100.0 + 47.0);
const float calibrationFactor = 1.12; 
const float minVoltage = 6.0; 
const float maxVoltage = 8.4;
