#include <esp_now.h>
#include <WiFi.h>

uint8_t broadcastAddress[4][6] = {
  {0x80, 0x7D, 0x3A, 0xEA, 0xB1, 0x98}, // esp32 ext ant R1
  {0xE8, 0x6B, 0xEA, 0xD4, 0xA3, 0xA0}, // esp32 v1 microUSB 
  {0x80, 0x7D, 0x3A, 0xB9, 0x1F, 0xB4}, // remote2
  {0x14, 0x33, 0x5C, 0x02, 0x34, 0x58}  // remote3
};

typedef struct DatatoSend {
  int rpmData[2];
  float voltData[3];
  int pressureData;
} DatatoSend;

DatatoSend sendData; //struct to store sent data. sendData.rpmData[0]

typedef struct struct_message {
    bool stat[16];
    int joyData[4];
    uint8_t remoteIndex = 1;

//    char address[18];
} struct_message;
struct_message incomingData; // struct to store received data
uint8_t storedMac;
bool connectOk;
bool failsafeTriggered = false;
bool encoderOn;

//========================================ESPNOW DATA RECEIVING FUNCTIONS======================================//

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingLocal, int len) {
    DEBUG_PRINTLN("OnDataRecv CALLED");
    memcpy(&incomingData, incomingLocal, sizeof(incomingData));
    String dataLine = String();
    for (int i = 0; i < 4; i++) {  dataLine += String(incomingData.joyData[i]) + ","; }
    for (int i = 0; i < 15; i++) { dataLine += String(incomingData.stat[i]) + ",";    }
    
    dataLine += String(incomingData.remoteIndex);
//    dataLine += incomingData.address;
//    dataLine.remove(dataLine.length() - 1); 
    Serial2.println(dataLine);
    //serial monitor remote
    DEBUG_PRINT("[BTN] ");
    if (incomingData.stat[4])  DEBUG_PRINT("SQUARE ");
    if (incomingData.stat[5])  DEBUG_PRINT("CROSS ");
    if (incomingData.stat[6])  DEBUG_PRINT("CIRCLE ");
    if (incomingData.stat[7])  DEBUG_PRINT("TRIANGLE ");
    if (incomingData.stat[0])  DEBUG_PRINT("UP ");
    if (incomingData.stat[1])  DEBUG_PRINT("RIGHT ");
    if (incomingData.stat[2])  DEBUG_PRINT("DOWN ");
    if (incomingData.stat[3])  DEBUG_PRINT("LEFT ");
    if (incomingData.stat[8])  DEBUG_PRINT("R1 ");
    if (incomingData.stat[9])  DEBUG_PRINT("R2 ");
    if (incomingData.stat[12]) DEBUG_PRINT("L1 ");
    if (incomingData.stat[13]) DEBUG_PRINT("L2 ");
    if (incomingData.stat[10]) DEBUG_PRINT("R3 ");
    if (incomingData.stat[11]) DEBUG_PRINT("L3 ");
    if (incomingData.stat[14]) DEBUG_PRINT("SWITCH ");
    if (incomingData.stat[15]) DEBUG_PRINT("LED_BTN ");
    DEBUG_PRINTLN();

    DEBUG_PRINT("\n[RX ANALOG] ");
    DEBUG_PRINT("LX="); DEBUG_PRINT(incomingData.joyData[0]);
    DEBUG_PRINT(" LY="); DEBUG_PRINT(incomingData.joyData[1]);
    DEBUG_PRINT(" RX="); DEBUG_PRINT(incomingData.joyData[2]);
    DEBUG_PRINT(" RY="); DEBUG_PRINT(incomingData.joyData[3]);
    DEBUG_PRINTLN();
    
    // DEBUG_PRINTLN("Serial Sent Data: " + dataLine);
    check_EEPROM();
}

//========================================ESPNOW DATA SENDING FUNCTIONS======================================//


//esp_now_peer_info_t peerInfo;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {  
//  DEBUG_PRINT("----Last Packet Send Status:");
  connectOk = (status == ESP_NOW_SEND_SUCCESS);
  if(connectOk){
    DEBUG_PRINTLN("Delivery Success");

  }else{
    DEBUG_PRINTLN("Delivery Fail");
  }
}

void startComms(){
  if (esp_now_init() != ESP_OK) { DEBUG_PRINTLN("Error initializing ESP-NOW"); return; }
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);
  
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastAddress[incomingData.remoteIndex], 6);
  //memcpy(peerInfo.peer_addr, broadcastAddress[3], 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;      
  esp_now_add_peer(&peerInfo);
  DEBUG_PRINTLN("Peer for REMOTE 3 added");
  //String macStr = macToString(broadcastAddress[incomingData.remoteIndex]);
  //DEBUG_PRINT("Sending Data To: ");  DEBUG_PRINTLN(macStr);
}

void dataSent() {  
    static uint32_t pM;
    uint32_t cM = millis();
    String dataStr = String();
    if ((cM - pM > 1500) || encoderOn ) {
//       sendData.rpmData[0] = random(0, 40000);
//       sendData.rpmData[1] = random(0, 40000);
//       sendData.voltData[0] = random(9, 12);
//       sendData.voltData[1] = random(12, 16);
//       sendData.voltData[2] = random(18, 24);
       sendData.pressureData = random(0, 600);

       for (int i = 0; i < 2; i++) {  dataStr += String(sendData.rpmData[i]) + ","; }
       for (int i = 0; i < 3; i++) {  dataStr += String(sendData.voltData[i]) + ","; }
       dataStr += String(sendData.pressureData) + " - ";
       DEBUG_PRINT(dataStr);
       esp_err_t result = esp_now_send(broadcastAddress[incomingData.remoteIndex], (uint8_t*)&sendData, sizeof(sendData));
       //esp_err_t result = esp_now_send(broadcastAddress[3], (uint8_t*)&sendData, sizeof(sendData));
       if (result == ESP_OK)  DEBUG_PRINT("SENT SUCCESS - ");
       else  DEBUG_PRINT("ERROR - ");
      
       DEBUG_PRINTLN(storedMac); 
//       String macStr = macToString(broadcastAddress[incomingData.remoteIndex]);
//       DEBUG_PRINTLN(macStr);
       pM = cM;    
    }
}

void failSafeCheck(struct_message &recvData) {
    static struct_message lastReceivedData;
    static unsigned long lastReceiveTime = 0;

    bool dataChanged = false;
    for (int i = 0; i < 4; i++) {
        if (recvData.joyData[i] != lastReceivedData.joyData[i]) {
            dataChanged = true;
            break;
        }
    }
    if (!dataChanged) {
        for (int i = 0; i < 15; i++) {
            if (recvData.stat[i] != lastReceivedData.stat[i]) {
                dataChanged = true;
                break;
            }
        }
    }
    if (dataChanged) {
        lastReceiveTime = millis();
        lastReceivedData = recvData; 
        failsafeTriggered = false;
    }
    if (!failsafeTriggered && (millis() - lastReceiveTime >= 2000)) {
        bool joystickMoved = false;
        bool buttonPressed = false;
        for (int i = 0; i < 4; i++) {
            if (lastReceivedData.joyData[i] != 0) {
                joystickMoved = true;
                break;
            }
        }
        for (int i = 0; i < 15; i++) {
            if (!lastReceivedData.stat[i]) {
                buttonPressed = true;
                break;
            }
        }
        if (joystickMoved || buttonPressed) {
            for (int i = 0; i < 4; i++) recvData.joyData[i] = 0;
            for (int i = 0; i < 15; i++) recvData.stat[i] = true;
            String dataLine;
            
            for (int i = 0; i < 4; i++)  dataLine += String(recvData.joyData[i]) + ",";
            for (int i = 0; i < 15; i++) dataLine += String(recvData.stat[i]) + ",";
            dataLine += String(recvData.remoteIndex);
            Serial2.println(dataLine);
            DEBUG_PRINTLN("FAILSAFE: RESETTING THE SERIAL DATA");
            DEBUG_PRINTLN("Serial Sent Data: " + dataLine);

            failsafeTriggered = true;
        }
    }
}


//String macToString(const uint8_t *mac) {
//  char macChr[18];
//  sprintf(macChr, "%02X:%02X:%02X:%02X:%02X:%02X",
//          mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
//  return String(macChr);
//}
