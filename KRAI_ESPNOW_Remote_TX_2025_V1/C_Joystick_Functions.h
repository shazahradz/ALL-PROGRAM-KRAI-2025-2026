  //=====================================Joystick Calibration & read FUNC (ADS1115)=====================================//
  void calibrateJoysticks() {
    DEBUG_PRINTLN("JOYSTICK CALIBRATION (ADS1115)");

    // Init ADS1115
    if (!ads.begin(0x48)) {
      DEBUG_PRINTLN("ERROR: ADS1115 NOT DETECTED!");
      while (1);
    }
    ads.setGain(GAIN_ONE);
    
    DEBUG_PRINTLN("ADS1115 initialized");
    DEBUG_PRINT("Joystick Centers: ");
    DEBUG_PRINT(joyCenter[0]); DEBUG_PRINT(", ");
    DEBUG_PRINT(joyCenter[1]); DEBUG_PRINT(", ");
    DEBUG_PRINT(joyCenter[2]); DEBUG_PRINT(", ");
    DEBUG_PRINTLN(joyCenter[3]);
    
    DEBUG_PRINTLN("CALIBRATION COMPLETE");
  }

  int readJoystickADS(uint8_t ch) {
    int raw = ads.readADC_SingleEnded(ch);
    int val = raw - joyCenter[ch];

    if (val > 0) {
      val = map(val, 0, joyMax[ch] - joyCenter[ch], 0, 4095);
    } else {
      val = map(val, joyMin[ch] - joyCenter[ch], 0, -4095, 0);
    }

    val = constrain(val, -4095, 4095);
    if (abs(val) < deadzone) val = 0;

    return val;
  }

  void joystickRead(){
    // Read from ADS1115 (4 channels via I2C)
    // A0 -> XL (joy1X)
    // A1 -> YL (joy1Y)
    // A2 -> XR (joy2X)
    // A3 -> YR (joy2Y)
    
    sendData.joyData[0] = readJoystickADS(1); // XL
    sendData.joyData[1] = readJoystickADS(0); // YL
    sendData.joyData[2] = readJoystickADS(3); // XR
    sendData.joyData[3] = readJoystickADS(2); // YR
    
    // Update position untuk OLED display
    joy1XPos = map(sendData.joyData[0], 4095, -4095, joy1XBox - boxSize / 2, joy1XBox + boxSize / 2);
    joy1YPos = map(sendData.joyData[1], 4095, -4095, joy1YBox - boxSize / 2, joy1YBox + boxSize / 2);
    joy2XPos = map(sendData.joyData[2], 4095, -4095, joy2XBox - boxSize / 2, joy2XBox + boxSize / 2);
    joy2YPos = map(sendData.joyData[3], 4095, -4095, joy2YBox - boxSize / 2, joy2YBox + boxSize / 2);

        // balik X kalau kanan-kiri kebalik
    // joy1XPos = 128 - joy1XPos;
    // joy2XPos = 128 - joy2XPos;

    // // kalau atas-bawah juga kebalik, tambahkan ini:
    // joy1YPos = 64 - joy1YPos;
    // joy2YPos = 64 - joy2YPos;

  }
