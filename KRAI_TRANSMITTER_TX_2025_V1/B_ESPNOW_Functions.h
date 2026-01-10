void ledBlink(int wait){
  static uint32_t blinkMillis;
  if(millis() - blinkMillis >= wait){
    blinkMillis=millis();
    digitalWrite(5, !digitalRead(5));    
  }
}

//========================================ESPNOW DATA SENDING FUNCTIONS======================================//
bool connectOk = false;
uint8_t connStat = false;
unsigned long lastActiveTime = 0;      
const unsigned long timeout = 1000;    
bool isSending = false; 

esp_now_peer_info_t peerInfo;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {  
  DEBUG_PRINT("----Last Packet Send Status:");
  connectOk = (status == ESP_NOW_SEND_SUCCESS);
  if(connectOk){
    DEBUG_PRINTLN("Delivery Success----");  // 
  }else{
    DEBUG_PRINTLN("Delivery Fail----");     // 
  }
}

void startSend(){
  if (esp_now_init() != ESP_OK) { DEBUG_PRINTLN("Error initializing ESP-NOW"); return; }
  esp_now_register_send_cb(OnDataSent);
  memcpy(peerInfo.peer_addr, broadcastAddress[macIndex], 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;      
  if (esp_now_add_peer(&peerInfo) != ESP_OK){ DEBUG_PRINTLN("Failed to add peer"); return; }
}

//================= PCF8575 Functions =================//
void scanPCF8575() {
  DEBUG_PRINTLN("Scanning PCF8575...");
  for (uint8_t addr = 0x20; addr <= 0x27; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      PCF8575_ADDR = addr;
      DEBUG_PRINT("PCF8575 found at 0x");
      DEBUG_PRINTLN(addr, HEX);
      return;
    }
  }
  DEBUG_PRINTLN("ERROR: PCF8575 NOT FOUND!");
}

uint16_t readPCF8575() {
  Wire.requestFrom(PCF8575_ADDR, (uint8_t)2);
  uint16_t data = 0xFFFF;
  
  if (Wire.available() == 2) {
    data = Wire.read();
    data |= Wire.read() << 8;
  }
  return data;
}

void dataSent() {
  unsigned long currentTime = millis();
  bool isButtonPressed = false;      
  bool isJoystickMoved = false;      

  // Read buttons from PCF8575 (16-bit)
  pcfData = readPCF8575();
  
  // Direct mapping PCF8575 -> sendData.stat
  sendData.stat[0]  = !bitRead(pcfData, 0);   // UP
  sendData.stat[1]  = !bitRead(pcfData, 1);   // RIGHT
  sendData.stat[2]  = !bitRead(pcfData, 2);   // DOWN
  sendData.stat[3]  = !bitRead(pcfData, 3);   // LEFT
  sendData.stat[4]  = !bitRead(pcfData, 4);   // SQUARE
  sendData.stat[5]  = !bitRead(pcfData, 5);   // CROSS
  sendData.stat[6]  = !bitRead(pcfData, 6);   // CIRCLE
  sendData.stat[7]  = !bitRead(pcfData, 7);   // TRIANGLE
  sendData.stat[8]  = !bitRead(pcfData, 8);   // R1
  sendData.stat[9]  = !bitRead(pcfData, 9);   // R2
  sendData.stat[10] = !bitRead(pcfData, 11);  // R3
  sendData.stat[11] = !bitRead(pcfData, 10);  // L3
  sendData.stat[12] = !bitRead(pcfData, 13);  // L1
  sendData.stat[13] = !bitRead(pcfData, 14);  // L2
  sendData.stat[14] = !bitRead(pcfData, 15);  // SWITCH
  sendData.stat[15] = !bitRead(pcfData, 12);  // LED_BTN

  // Check button activity
  for (uint8_t i = 0; i < 16; i++) {
    if (sendData.stat[i] == true){
      isButtonPressed = true;  
      DEBUG_PRINT("Button "); DEBUG_PRINT(i); DEBUG_PRINTLN(" is pressed.");
      break;  // ✅ TAMBAH break untuk efisiensi
    }
  }

  // Check joystick activity
  for (uint8_t i = 0; i < 4; i++) {
    if (sendData.joyData[i] != 0) {           
      isJoystickMoved = true;              
      DEBUG_PRINT("Joystick axis "); DEBUG_PRINT(i); DEBUG_PRINT(" moved.");
      DEBUG_PRINT(" -val: "); DEBUG_PRINT(sendData.joyData[i]); DEBUG_PRINTLN("  - ");
      break;  // ✅ TAMBAH break untuk efisiensi
    }
  }

  if (isButtonPressed || isJoystickMoved) {
    lastActiveTime = currentTime;            
    isSending = true; 

        // TAMPILAN SERIAL
    DEBUG_PRINT("\n[BTN] ");
    if (sendData.stat[4])  DEBUG_PRINT("SQUARE ");
    if (sendData.stat[5])  DEBUG_PRINT("CROSS ");
    if (sendData.stat[6])  DEBUG_PRINT("CIRCLE ");
    if (sendData.stat[7])  DEBUG_PRINT("TRIANGLE ");
    if (sendData.stat[0])  DEBUG_PRINT("UP ");
    if (sendData.stat[1])  DEBUG_PRINT("RIGHT ");
    if (sendData.stat[2])  DEBUG_PRINT("DOWN ");
    if (sendData.stat[3])  DEBUG_PRINT("LEFT ");
    if (sendData.stat[8])  DEBUG_PRINT("R1 ");
    if (sendData.stat[9])  DEBUG_PRINT("R2 ");
    if (sendData.stat[12]) DEBUG_PRINT("L1 ");
    if (sendData.stat[13]) DEBUG_PRINT("L2 ");
    if (sendData.stat[10]) DEBUG_PRINT("R3 ");
    if (sendData.stat[11]) DEBUG_PRINT("L3 ");
    if (sendData.stat[14]) DEBUG_PRINT("SWITCH ");
    if (sendData.stat[15]) DEBUG_PRINT("LED_BTN ");

    DEBUG_PRINT("\n[ANALOG] ");
    DEBUG_PRINT("LX="); DEBUG_PRINT(sendData.joyData[0]);
    DEBUG_PRINT(" LY="); DEBUG_PRINT(sendData.joyData[1]);
    DEBUG_PRINT(" RX="); DEBUG_PRINT(sendData.joyData[2]);
    DEBUG_PRINT(" RY="); DEBUG_PRINT(sendData.joyData[3]);
    DEBUG_PRINTLN();
                 
  } else if (currentTime - lastActiveTime >= timeout) {
    isSending = false;                       
  }

  if (isSending) { 
    static uint32_t pM;
    uint32_t cM = millis();
    if (cM - pM > 45) {
      esp_err_t result = esp_now_send(broadcastAddress[macIndex], (uint8_t*)&sendData, sizeof(sendData));
      connStat = (connectOk ? 2 : 0);
      if (result == ESP_OK) DEBUG_PRINTLN("Sent with success!");
      else DEBUG_PRINTLN("Error sending the data.");
      pM = cM;   
    }
    ledBlink(250);
  } 
  else {
    DEBUG_PRINTLN("No activity, no data sent.");
    connStat = 1;
    ledBlink(1000);
  }
}

//========================================ESPNOW DATA RECEIVING FUNCTIONS======================================//
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingLocal, int len) {
  memcpy(&incomingData, incomingLocal, sizeof(incomingData));
  DEBUG_PRINT("Bytes received: ");
  DEBUG_PRINTLN(len);
}

void startRecv(){
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
}
