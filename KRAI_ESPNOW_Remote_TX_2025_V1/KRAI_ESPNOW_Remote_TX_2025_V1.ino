/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------DS2 CONTROLLER WITH ESP32 & ESPNOW COMMUNICATION PROTOCOL---------------------*/
/*--------------------------------------Source Code by LEXARGA-24 TEAM-------------------------------------*/
/*-----------------------------------Modified & Adapted by LEXARGA-24 TEAM---------------------------------*/
/*---------------------------------------------------V3.6.3------------------------------------------------*/
/*------------------------------------Specified for Remote 3(gen 3 remote)---------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*------------------------------------LAST UPDATE AT 02:36:00, 2 JUL 25-----------------------------------*/

// Define DEBUG to enable debugging; comment it out to disable
#define DEBUG

#ifdef DEBUG
  #define DEBUG_PRINT(...) Serial.print(__VA_ARGS__)
  #define DEBUG_PRINTLN(...) Serial.println(__VA_ARGS__)
  #define DEBUG_BEGIN(baud) Serial.begin(baud)
#else
  #define DEBUG_PRINT(...)    // Do nothing
  #define DEBUG_PRINTLN(...)  // Do nothing
  #define DEBUG_BEGIN(baud)   // Do nothing
#endif

#include "A_GlobalVariables.h"
#include "B_ESPNOW_Functions.h"
#include "C_Joystick_Functions.h"
#include "D_Oled_Functions.h"
#include "E_WebServer_Update.h"

//==========================================MAIN PROGRAM (SETUP & LOOP)=====================================//
uint16_t readPCFonce() {
  Wire.beginTransmission(PCF8575_ADDR);
  Wire.write(0xFF);          // set semua jadi input (HIGH)
  Wire.write(0xFF);
  Wire.endTransmission();

  Wire.requestFrom(PCF8575_ADDR, (uint8_t)2);
  uint8_t lowB  = Wire.read();
  uint8_t highB = Wire.read();

  uint16_t data = (highB << 8) | lowB;
  return data;
}
void setup(){
  DEBUG_BEGIN(115200);
  delay(1000);  
  DEBUG_PRINTLN("\n\n========== REMOTE V3.7.0 STARTING ==========");
  
  WiFi.mode(WIFI_STA); 
  sendData.remoteIndex = 3;
  DEBUG_PRINT("Index MAC : ");
  DEBUG_PRINTLN(sendData.remoteIndex);
  EEPROM.begin(256);
  reload_EEPROM();
  
  // Init I2C untuk ADS1115, PCF8575, dan OLED
  Wire.begin(21, 22);
  DEBUG_PRINTLN("I2C Bus initialized");
  
  //scanPCF8575() KE SINI - SEBELUM calibrateJoysticks()
  DEBUG_PRINTLN("Initializing PCF8575...");
  scanPCF8575();
  if (PCF8575_ADDR != 0xFF) {
    Wire.beginTransmission(PCF8575_ADDR);
    Wire.write(0xFF);
    Wire.write(0xFF);
    Wire.endTransmission();
    DEBUG_PRINTLN("PCF8575 initialized OK");
  } else {
    DEBUG_PRINTLN("WARNING: PCF8575 NOT FOUND - buttons disabled!");
  }
  
  // Init OLED
  DEBUG_PRINTLN("Initializing OLED...");
 // u8g2.setI2CAddress(0x3C); GAK DIPAKAI KARENA UDAH KEBACA
  u8g2.begin();
  DEBUG_PRINTLN("OLED initialized OK");
  
  // Pin GPIO
  pinMode(33, INPUT_PULLUP);
  pinMode(2, INPUT_PULLUP);
  pinMode(voltPin, INPUT);
  pinMode(5, OUTPUT);
  DEBUG_PRINTLN("GPIO pins configured");
  
  // Init ADS1115
  calibrateJoysticks();

  // Splash screen
  DEBUG_PRINTLN("Displaying splash screen...");
  dispSplashLogo();
    
  DEBUG_PRINTLN("Setup complete!");

  // 1) BACA PCF SATU KALI DI SINI
  uint16_t pcfData = readPCFonce();              // <-- fungsi yang baca 16‑bit dari PCF
  sendData.stat[10] = !bitRead(pcfData, 11);     // R3 sama persis kaya di loop

  DEBUG_PRINT("R3 / stat[10] at boot: ");
  DEBUG_PRINTLN(sendData.stat[10]);

  // 2) BARU TENTUKAN MODE
  bool otaMode = sendData.stat[10];              // kalau mau kebalik: !sendData.stat[10]

  if (!otaMode) {                                // NORMAL MODE
    settingMode = false;
    startSend();
    startRecv();
    DEBUG_PRINTLN(">>>>> NORMAL MODE - ESP-NOW Active <<<<<");
  } else {                                       // OTA MODE
    settingMode = true;
    batteryCheck();
    StartOTA();
    displaySetPage();
    DEBUG_PRINTLN(">>>>> SETTING MODE - OTA Active <<<<<");
  }

  DEBUG_PRINTLN("Entering main loop...\n");
}
void loop() {
  if (settingMode){
    ledBlink(500); 
    batteryCheck();
  }else{
    checkLatency(true);
    joystickRead();
    dataSent();
    
    static bool lastSwitch = false;
    bool curSwitch = sendData.stat[14];  // tombol dari PCF

    if (curSwitch && !lastSwitch) {      // tepi naik
    oledMode = !oledMode;
    }
    lastSwitch = curSwitch;

    if (oledMode == 0) OledDisplaySend();
    else              OledMonitorSend();

    // if (digitalRead(2)) OledMonitorSend();
    // else OledDisplaySend();
    
    batteryCheck();
    checkLatency(false);
    
    // tambah print biar tau loop jalan
    static uint32_t lastPrint = 0;
    if(millis() - lastPrint > 2000){
      lastPrint = millis();
      DEBUG_PRINTLN("Loop running OK...");
    }
  }
}

/*void setup(){
  DEBUG_BEGIN(115200);
  WiFi.mode(WIFI_STA); 
//  String addrStr = String(WiFi.macAddress());
//  strcpy(sendData.address, addrStr.c_str());
  sendData.remoteIndex = 3;

  EEPROM.begin(256);
  reload_EEPROM();
  
  u8g2.setI2CAddress(0x78);
  u8g2.begin(); // start the u8g2 library

  // for(uint8_t i = 0; i < 15; i++){
  //   pinMode(button[i], INPUT_PULLUP);
  // }

  pinMode(2, INPUT_PULLUP);
  pinMode(voltPin, INPUT);
  pinMode(5, OUTPUT);
  
  calibrateJoysticks();
  dispSplashLogo();
//  batteryCheck();
  
  if (digitalRead(33)) { 
    settingMode = false; 
    startSend();
    startRecv();
//    OledDisplaySend();
  }else{
    settingMode = true;
    batteryCheck();
    StartOTA();
    displaySetPage();
  }  
}

void loop() {
  if (settingMode){
    ledBlink(500); 
    batteryCheck();
  }else{
    checkLatency(true);
    joystickRead();
    dataSent();
    if (digitalRead(2)) OledMonitorSend();
    else OledDisplaySend();
    batteryCheck();
//    delay(10);
    checkLatency(false);
  }
}
*/
//===========================================Voltage reading FUNC===============================================//
float readings[15]; 
uint8_t readIndex = 0;
uint8_t totalSamples = 0;
unsigned long lastOutputTime = 0;
unsigned long lastSampleTime = 0;
void batteryCheck() {
  if (millis() - lastSampleTime >= 100) {
    lastSampleTime = millis();

    float vOut = (analogRead(voltPin) / 4095.0) * 3.3;
    float newVoltage = (vOut / voltageDividerFactor) * calibrationFactor;

    readings[readIndex] = newVoltage;
    readIndex = (readIndex + 1) % 15;
    if (totalSamples < 15) totalSamples++;
  }

  if ((millis() - lastOutputTime >= 1500)) {
    lastOutputTime = millis();
    float sum = 0;
    for (int i = 0; i < totalSamples; i++) {
      sum += readings[i];
    }
    batteryVoltage = sum / totalSamples;
//    batteryPercentage = (batteryVoltage - minVoltage) / (maxVoltage - minVoltage) * 100;
//    batteryPercentage = constrain(batteryPercentage, 0, 100);

    DEBUG_PRINT("                                             Battery Voltage: ");
    DEBUG_PRINT(batteryVoltage, 2);
    DEBUG_PRINT(" V, Percentage: ");
//    DEBUG_PRINT(batteryPercentage, 0);
//    DEBUG_PRINTLN(" %");
  }
}

//=============================================Blink ISR FUNC===============================================//

//void ledBlink(int wait){
//  static uint32_t blinkMillis;
//  if(millis() - blinkMillis >= wait){
//    blinkMillis=millis();
//    digitalWrite(5, !digitalRead(5));    
//  }
//}

void checkLatency(bool mod){  
  static unsigned long startTime;
  if (mod){ startTime = millis(); }
  else{
    unsigned long endTime = millis(); 
    periodMs = endTime - startTime;
    frequencyHz = 1000.0 / periodMs;
    DEBUG_PRINT("--F : "); DEBUG_PRINT(frequencyHz); DEBUG_PRINT(" Hz");
    DEBUG_PRINT("--T : ");   DEBUG_PRINT(periodMs); DEBUG_PRINTLN(" mS");
  }
}
