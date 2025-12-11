#include <Wire.h>
#include <Adafruit_ADS1X15.h>

Adafruit_ADS1115 ads;

const float LSB_V = 0.125F / 1000.0F; // GAIN_ONE
const int CAL_SAMPLES = 200; // jumlah sample untuk kalibrasi
const int AVG_N = 8;
int idx = 0;
int buf0[AVG_N]={0}, buf1[AVG_N]={0}, buf2[AVG_N]={0}, buf3[AVG_N]={0};
long sum0=0, sum1=0, sum2=0, sum3=0;

// hasil kalibrasi (akan diisi)
int minVal[4] = {32767,32767,32767,32767};
int maxVal[4] = {0,0,0,0};
int centerVal[4] = {16384,16384,16384,16384};
int deadzone = 150; // toleransi sekitar center (adjust)

void setup() {
  Serial.begin(115200);
  Wire.begin();
  if (!ads.begin(0x48)) {
    Serial.println("ADS1115 tidak terdeteksi!");
    while (1);
  }
  ads.setGain(GAIN_ONE);
  Serial.println("Mulai kalibrasi: jangan gerakkan joystick dulu (tunggu)...");
  delay(1000);

  // simple auto-calibration: mintain min/max while user moves joystick to extremes
  Serial.println("Sekarang gerakkan semua joystick ke semua ujung (sekitar 5-8 detik)...");
  unsigned long t0 = millis();
  while (millis() - t0 < 7000) { // 7 detik
    int16_t r0 = ads.readADC_SingleEnded(0);
    int16_t r1 = ads.readADC_SingleEnded(1);
    int16_t r2 = ads.readADC_SingleEnded(2);
    int16_t r3 = ads.readADC_SingleEnded(3);
    if (r0 < minVal[0]) minVal[0] = r0;
    if (r1 < minVal[1]) minVal[1] = r1;
    if (r2 < minVal[2]) minVal[2] = r2;
    if (r3 < minVal[3]) minVal[3] = r3;
    if (r0 > maxVal[0]) maxVal[0] = r0;
    if (r1 > maxVal[1]) maxVal[1] = r1;
    if (r2 > maxVal[2]) maxVal[2] = r2;
    if (r3 > maxVal[3]) maxVal[3] = r3;
    delay(20);
  }

  // compute centers
  for (int i=0;i<4;i++){
    centerVal[i] = (minVal[i] + maxVal[i]) / 2;
  }

  Serial.println("Kalibrasi selesai. Nilai min/max/center:");
  for (int i=0;i<4;i++){
    Serial.print("A"); Serial.print(i); Serial.print(": min="); Serial.print(minVal[i]);
    Serial.print(" max="); Serial.print(maxVal[i]); Serial.print(" center="); Serial.println(centerVal[i]);
  }
  Serial.println("Mulai pembacaan...");
}

float toVoltage(int32_t raw){ return raw * LSB_V; }

// map raw -> -100..+100 berdasarkan kalibrasi
int mapToPercent(int raw, int axis){
  int c = centerVal[axis];
  int mn = minVal[axis];
  int mx = maxVal[axis];

  if (raw > c + deadzone){ // pos side
    float denom = (float)(mx - c);
    if (denom <= 0.1) return 0;
    float frac = (raw - c) / denom; // 0..1
    if (frac > 1.0) frac = 1.0;
    return (int)(frac * 100.0); // 0..100
  } else if (raw < c - deadzone){ // neg side
    float denom = (float)(c - mn);
    if (denom <= 0.1) return 0;
    float frac = (c - raw) / denom; // 0..1
    if (frac > 1.0) frac = 1.0;
    return (int)(-frac * 100.0); // -100..0
  } else {
    return 0; // deadzone -> treat as zero
  }
}

void loop(){
  int16_t r0 = ads.readADC_SingleEnded(0);
  int16_t r1 = ads.readADC_SingleEnded(1);
  int16_t r2 = ads.readADC_SingleEnded(2);
  int16_t r3 = ads.readADC_SingleEnded(3);

  // moving average
  sum0 -= buf0[idx]; buf0[idx] = r0; sum0 += buf0[idx];
  sum1 -= buf1[idx]; buf1[idx] = r1; sum1 += buf1[idx];
  sum2 -= buf2[idx]; buf2[idx] = r2; sum2 += buf2[idx];
  sum3 -= buf3[idx]; buf3[idx] = r3; sum3 += buf3[idx];
  idx = (idx + 1) % AVG_N;

  int ar0 = (int)(sum0 / AVG_N);
  int ar1 = (int)(sum1 / AVG_N);
  int ar2 = (int)(sum2 / AVG_N);
  int ar3 = (int)(sum3 / AVG_N);

  Serial.print("XL(A0): "); Serial.print(ar0); Serial.print(" | "); Serial.print(toVoltage(ar0),4); Serial.print(" V");
  Serial.print(" | %:"); Serial.print(mapToPercent(ar0,0));
  Serial.print("  ||  ");

  Serial.print("YL(A1): "); Serial.print(ar1); Serial.print(" | "); Serial.print(toVoltage(ar1),4); Serial.print(" V");
  Serial.print(" | %:"); Serial.print(mapToPercent(ar1,1));
  Serial.print("  ||  ");

  Serial.print("XR(A2): "); Serial.print(ar2); Serial.print(" | "); Serial.print(toVoltage(ar2),4); Serial.print(" V");
  Serial.print(" | %:"); Serial.print(mapToPercent(ar2,2));
  Serial.print("  ||  ");

  Serial.print("YR(A3): "); Serial.print(ar3); Serial.print(" | "); Serial.print(toVoltage(ar3),4); Serial.print(" V");
  Serial.print(" | %:"); Serial.println(mapToPercent(ar3,3));

  delay(80);
}
