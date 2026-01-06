#include <Wire.h>
#include <Adafruit_ADS1X15.h>

/* ================= ADS1115 ================= */
Adafruit_ADS1115 ads;

/*
  ADS1115 Channel Mapping
  A0 -> XL
  A1 -> YL
  A2 -> XR
  A3 -> YR
*/

// ===== HASIL KALIBRASI =====
int joyCenter[4] = {13055, 13119, 12815, 13165};
int joyMin[4]    = {11, 10, 11, 9};
int joyMax[4]    = {26319, 26139, 26326, 26325};

int joyData[4];
int deadzone = 150;

/* ================= PCF8575 ================= */
uint8_t PCF8575_ADDR = 0xFF;
uint16_t pcfData = 0xFFFF;

/* ================= PCF SCAN ================= */
void scanPCF8575() {
  for (uint8_t addr = 0x20; addr <= 0x27; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      PCF8575_ADDR = addr;
      Serial.print("PCF8575 ditemukan di 0x");
      Serial.println(addr, HEX);
      return;
    }
  }
}

/* ================= READ PCF ================= */
uint16_t readPCF8575() {
  Wire.requestFrom(PCF8575_ADDR, (uint8_t)2);
  uint16_t data = 0xFFFF;

  if (Wire.available() == 2) {
    data = Wire.read();
    data |= Wire.read() << 8;
  }
  return data;
}

/* ================= READ ADS ================= */
int readJoystickAnalog(uint8_t ch) {
  int raw = ads.readADC_SingleEnded(ch);
  int val = raw - joyCenter[ch];

  if (val > 0) {
    val = map(val, 0,
              joyMax[ch] - joyCenter[ch],
              0, 4095);
  } else {
    val = map(val,
              joyMin[ch] - joyCenter[ch],
              0,
              -4095, 0);
  }

  val = constrain(val, -4095, 4095);
  if (abs(val) < deadzone) val = 0;

  return val;
}

/* ================= SETUP ================= */
void setup() {
  Serial.begin(115200);
  Wire.begin();   // UNO: SDA A4, SCL A5

  /* ===== ADS1115 INIT ===== */
  if (!ads.begin(0x48)) {
    Serial.println("ADS1115 TIDAK TERDETEKSI!");
    while (1);
  }
  ads.setGain(GAIN_ONE);
  Serial.println("ADS1115 READY");

  /* ===== PCF8575 INIT ===== */
  Serial.println("Scan PCF8575...");
  scanPCF8575();

  if (PCF8575_ADDR == 0xFF) {
    Serial.println("PCF8575 TIDAK DITEMUKAN!");
    while (1);
  }

  Wire.beginTransmission(PCF8575_ADDR);
  Wire.write(0xFF);
  Wire.write(0xFF);
  Wire.endTransmission();

  Serial.println("PCF8575 READY");
  Serial.println("SYSTEM READY\n");
}

/* ================= LOOP ================= */
void loop() {

  /* ===== BACA ANALOG (ADS1115) ===== */
  joyData[0] = readJoystickAnalog(0); // XL
  joyData[1] = readJoystickAnalog(1); // YL
  joyData[2] = readJoystickAnalog(2); // XR
  joyData[3] = readJoystickAnalog(3); // YR

  Serial.print("ADS | ");
  Serial.print("XL: "); Serial.print(joyData[0]);
  Serial.print(" | YL: "); Serial.print(joyData[1]);
  Serial.print(" || XR: "); Serial.print(joyData[2]);
  Serial.print(" | YR: "); Serial.print(joyData[3]);

  /* ===== BACA BUTTON (PCF8575) ===== */
  pcfData = readPCF8575();

  int UP    = !bitRead(pcfData, 0);
  int RIGHT = !bitRead(pcfData, 1);
  int DOWN  = !bitRead(pcfData, 2);
  int LEFT  = !bitRead(pcfData, 3);
  

  int SQUARE   = !bitRead(pcfData, 4);
  int CROSS    = !bitRead(pcfData, 5);
  int CIRCLE   = !bitRead(pcfData, 6);
  int TRIANGLE = !bitRead(pcfData, 7);

  int R1 = !bitRead(pcfData, 8);
  int R2 = !bitRead(pcfData, 9);
  int R3 = !bitRead(pcfData, 11);

  int L3 = !bitRead(pcfData, 10);
  int LED_BTN = !bitRead(pcfData, 12);
  int L1 = !bitRead(pcfData, 13);
  int L2 = !bitRead(pcfData, 14);
  int SWITCH_BTN = !bitRead(pcfData, 15);

  Serial.print(" || PCF | ");
  Serial.print("U:"); Serial.print(UP);
  Serial.print(" D:"); Serial.print(DOWN);
  Serial.print(" R:"); Serial.print(RIGHT);
  Serial.print(" L:"); Serial.print(LEFT);

  Serial.print(" | SQ:"); Serial.print(SQUARE);
  Serial.print(" X:"); Serial.print(CROSS);
  Serial.print(" O:"); Serial.print(CIRCLE);
  Serial.print(" T:"); Serial.print(TRIANGLE);

  Serial.print(" | R1:"); Serial.print(R1);
  Serial.print(" R2:"); Serial.print(R2);
  Serial.print(" R3:"); Serial.print(R3);

  Serial.print(" | L1:"); Serial.print(L1);
  Serial.print(" L2:"); Serial.print(L2);
  Serial.print(" L3:"); Serial.print(L3);
  Serial.print(" SW:"); Serial.print(SWITCH_BTN);
  Serial.print(" LED:"); Serial.print(LED_BTN);

  Serial.println();
  delay(40);
}
