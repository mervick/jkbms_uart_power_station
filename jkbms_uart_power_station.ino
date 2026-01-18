#include "JKBMSInterface.h"

JKBMSInterface bms(&Serial1);

const int RELAY_PIN = 3; 
const float CHARGE_COMPLETE_THRESHOLD = 0.8f; 
const float LOOP_DISCHARGE_THRESHOLD = -10.0f;

float MAX_CURRENT = 0.0f;
bool RELAY_LOCKED = false;
bool CHECK_DISCHARGE = true;

void setup() {
  Serial.begin(115200); 
  bms.begin(115200); 
}

void loop() {
  // If the system is locked (due to Full Charge or Protection), stay here forever
  if (RELAY_LOCKED) {
    return;
  }
  
  // Read BMS data as fast as possible
  bms.update();
  // bms.printSummary();

  if (bms.isDataValid()) {
    float current = bms.getCurrent() * -1; 
    
    // EMERGENCY LOOP DETECTION
    // Works only until the actual charging begins
    if (CHECK_DISCHARGE && current < LOOP_DISCHARGE_THRESHOLD) {
      pinMode(RELAY_PIN, OUTPUT);

      Serial.print("ALARM: Loop Detected! Discharge: ");
      Serial.println(current);
      RELAY_LOCKED = true; 
      return; 
    }

    // CHARGE COMPLETION CUTOFF
    // Detects end of charging cycle (100% SOC + Current drop) to disconnect power
    uint8_t soc = bms.getSOC();
    if (soc == 100 && current > 0 && MAX_CURRENT > current && current <= CHARGE_COMPLETE_THRESHOLD) {
      pinMode(RELAY_PIN, OUTPUT);

      Serial.println("Charger OFF PERMANENTLY (Full Charge).");
      RELAY_LOCKED = true;
      return;
    }

    // If the current has risen above the previous maximum (and it is > 0)
    if (current > MAX_CURRENT) {
      MAX_CURRENT = current;
      if (CHECK_DISCHARGE == true) {
        Serial.println("Charging started correctly. Loop check DISABLED.");
        CHECK_DISCHARGE = false;
      }
    }
  }

  if (CHECK_DISCHARGE) {
    delay(200);
  } else {
    delay(10000);
  }
}
