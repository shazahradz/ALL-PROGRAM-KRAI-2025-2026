#include <Wire.h>

#define PCF8575_ADDR 0x23

uint16_t pcfData = 0xFFFF; // semua HIGH (input + pull-up)

void setup() {
  Serial.begin(9600);
  // Wire.begin(21, 22); // SDA, SCL ESP32
  Wire.begin();

  // Set semua pin PCF8575 sebagai input (tulis HIGH)
  Wire.beginTransmission(PCF8575_ADDR);
  Wire.write(lowByte(pcfData));
  Wire.write(highByte(pcfData));
  Wire.endTransmission();

  Serial.println("PCF8575 Joystick Monitor Ready");
}

uint16_t readPCF8575() {
  Wire.requestFrom(PCF8575_ADDR, 2);
  uint16_t data = 0;

  if (Wire.available() == 2) {
    data = Wire.read();
    data |= Wire.read() << 8;
  }
  return data;
}

void loop() {
  pcfData = readPCF8575();

  // Tombol (dibalik: ditekan = 1, dilepas = 0)
  int UP       = !bitRead(pcfData, 0);  // P0
  int DOWN     = !bitRead(pcfData, 2);  // P1
  int RIGHT    = !bitRead(pcfData, 1);  // P2
  int LEFT     = !bitRead(pcfData, 3);  // P3

  int SQUARE   = !bitRead(pcfData, 4);  // P4
  int CROSS    = !bitRead(pcfData, 5);  // P5
  int CIRCLE   = !bitRead(pcfData, 6);  // P6
  int TRIANGLE = !bitRead(pcfData, 7);  // P7

  int R1 = !bitRead(pcfData, 8); // P8
  int R2 = !bitRead(pcfData, 9); // P9
  int R3 = !bitRead(pcfData, 10); // P10

  int SWITCH_BTN = !bitRead(pcfData, 15); // P11
  int LED_BTN    = !bitRead(pcfData, 12); // P12
  int L1         = !bitRead(pcfData, 13); // P13
  int L2         = !bitRead(pcfData, 14); // P14
  int L3         = !bitRead(pcfData, 11); // P15

  // Output ke Serial Monitor
  Serial.print("UP:"); Serial.print(UP);
  Serial.print(" DOWN:"); Serial.print(DOWN);
  Serial.print(" RIGHT:"); Serial.print(RIGHT);
  Serial.print(" LEFT:"); Serial.print(LEFT);

  Serial.print(" | SQ:"); Serial.print(SQUARE);
  Serial.print(" X:"); Serial.print(CROSS);
  Serial.print(" O:"); Serial.print(CIRCLE);
  Serial.print(" △:"); Serial.print(TRIANGLE);

  Serial.print(" | R1:"); Serial.print(R1);
  Serial.print(" R2:"); Serial.print(R2);
  Serial.print(" R3:"); Serial.print(R3);

  Serial.print(" | L1:"); Serial.print(L1);
  Serial.print(" | L2:"); Serial.print(L2);
  Serial.print(" | L3:"); Serial.print(L3);
  Serial.print(" SW:"); Serial.print(SWITCH_BTN);
  Serial.print(" LED:"); Serial.print(LED_BTN);

  Serial.println();

  delay(200);
}
