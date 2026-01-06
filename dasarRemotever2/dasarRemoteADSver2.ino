#include <Wire.h>
#include <Adafruit_ADS1X15.h>

Adafruit_ADS1115 ads;

/*
  Channel mapping ADS1115
  A0 -> XL
  A1 -> YL
  A2 -> XR
  A3 -> YR
*/

// ===== HASIL KALIBRASI KAMU =====
int joyCenter[4] = {
  13055,  // XL
  13119,  // YL
  12815,  // XR
  13165   // YR
};

int joyMin[4] = {
  11,     // XL
  10,     // YL
  11,     // XR
  9       // YR
};

int joyMax[4] = {
  26319,  // XL
  26139,  // YL
  26326,  // XR
  26325   // YR
};

// OUTPUT AKHIR (ANALOG VIRTUAL)
int joyData[4];

int deadzone = 150;   // deadzone sekitar tengah

// ================================

void setup() {
  Serial.begin(115200);
  Wire.begin();

  // alamat ADS1115 = 0x48
  if (!ads.begin(0x48)) {
    Serial.println("ADS1115 TIDAK TERDETEKSI!");
    while (1);
  }

  // ±4.096V (AMAN UNTUK 3.3V)
  ads.setGain(GAIN_ONE);

  Serial.println("ADS1115 READY - ANALOG MODE ONLY");
}

// ================================

int readJoystickAnalog(uint8_t ch) {

  int raw = ads.readADC_SingleEnded(ch);

  // seperti: analogRead() - center
  int val = raw - joyCenter[ch];

  if (val > 0) {
    val = map(val,
              0,
              joyMax[ch] - joyCenter[ch],
              0,
              4095);
  } else {
    val = map(val,
              joyMin[ch] - joyCenter[ch],
              0,
              -4095,
              0);
  }

  val = constrain(val, -4095, 4095);

  // deadzone
  if (abs(val) < deadzone) val = 0;

  return val;
}

// ================================

void loop() {

  joyData[0] = readJoystickAnalog(0); // XL
  joyData[1] = readJoystickAnalog(1); // YL
  joyData[2] = readJoystickAnalog(2); // XR
  joyData[3] = readJoystickAnalog(3); // YR

  Serial.print("XL: "); Serial.print(joyData[0]);
  Serial.print(" | YL: "); Serial.print(joyData[1]);
  Serial.print(" || XR: "); Serial.print(joyData[2]);
  Serial.print(" | YR: "); Serial.println(joyData[3]);

  delay(40);
}
