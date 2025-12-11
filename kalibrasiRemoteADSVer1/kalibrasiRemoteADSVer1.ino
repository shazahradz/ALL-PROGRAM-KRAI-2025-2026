#include <Wire.h>
#include <Adafruit_ADS1X15.h>

Adafruit_ADS1115 ads;

// ==== DATA KALIBRASI ====
// Joystick kiri
int YL_min = 10;
int YL_mid = 13119;
int YL_max = 26139;

int XL_min = 11;
int XL_mid = 13055;
int XL_max = 26319;

// Joystick kanan
int YR_min = 9;
int YR_mid = 13165;
int YR_max = 26325;

int XR_min = 11;
int XR_mid = 12815;
int XR_max = 26326;

// ==== NORMALISASI -100 sampai +100 ====
int normalize(int adc, int minVal, int midVal, int maxVal) {
  if (adc >= midVal) {
    return map(adc, midVal, maxVal, 0, 100);   // arah kanan / atas
  } else {
    return map(adc, minVal, midVal, -100, 0); // arah kiri / bawah
  }
}

void setup() {
  Serial.begin(115200);
  ads.begin(0x48);
  ads.setGain(GAIN_ONE);
}

void loop() {

  // *** BACA ADS1115 ***
  int adc_XL = ads.readADC_SingleEnded(0);  // A0
  int adc_YL = ads.readADC_SingleEnded(1);  // A1
  int adc_XR = ads.readADC_SingleEnded(2);  // A2
  int adc_YR = ads.readADC_SingleEnded(3);  // A3

  // *** KONVERSI KE PERSEN ***
  int XL_percent = normalize(adc_XL, XL_min, XL_mid, XL_max);
  int YL_percent = normalize(adc_YL, YL_min, YL_mid, YL_max);
  int XR_percent = normalize(adc_XR, XR_min, XR_mid, XR_max);
  int YR_percent = normalize(adc_YR, YR_min, YR_mid, YR_max);

  // *** TAMPILKAN ***
  Serial.print("XL: "); Serial.print(XL_percent);
  Serial.print(" | YL: "); Serial.print(YL_percent);
  Serial.print(" || XR: "); Serial.print(XR_percent);
  Serial.print(" | YR: "); Serial.println(YR_percent);

  delay(40);
}
